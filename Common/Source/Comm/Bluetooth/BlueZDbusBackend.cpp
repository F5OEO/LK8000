/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   BlueZDbusBackend.cpp
 *
 * Talks to bluetoothd directly over BlueZ's own D-Bus GATT client API
 * (org.bluez.Adapter1/Device1/GattService1/GattCharacteristic1), using
 * dbus-cxx, instead of going through gattlib. gattlib's own BlueZ/GDBus
 * backend turned out to have several real thread-safety bugs of its own
 * (a real device crash: "assertion 'G_IS_DBUS_PROXY (proxy)' failed"
 * followed by a SIGSEGV inside gattlib itself, plus a documented
 * non-thread-safe gattlib_adapter_scan_enable() -- see this file's git
 * history), on top of which LK8000 could only work around gattlib's races
 * rather than fix them. Talking to org.bluez directly removes that whole
 * layer.
 */

#include "BlueZDbusBackend.h"

#include <dbus-cxx.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include <unistd.h>

#include "gatt_utils.h"

namespace bluez_dbus_backend {

namespace {

constexpr const char* kBluezService = "org.bluez";
constexpr const char* kAdapterPath = "/org/bluez/hci0";
constexpr const char* kPropsIface = "org.freedesktop.DBus.Properties";
constexpr const char* kPropsChangedMember = "PropertiesChanged";
constexpr const char* kObjMgrIface = "org.freedesktop.DBus.ObjectManager";
constexpr const char* kAdapter1Iface = "org.bluez.Adapter1";
constexpr const char* kDevice1Iface = "org.bluez.Device1";
constexpr const char* kService1Iface = "org.bluez.GattService1";
constexpr const char* kChar1Iface = "org.bluez.GattCharacteristic1";

// The HM-10 and compatible bluetooth modules' data characteristic, used as
// the default write target for GattSensor::WriteData() -- matches the
// Android backend (Android/BluetoothGattClientPort.java's
// RX_TX_CHARACTERISTIC_UUID) and the previous gattlib backend.
constexpr uuid_t HM10_RX_TX_CHARACTERISTIC = bluetooth::gatt_uuid(0xFFE1);

using PropMap = std::map<std::string, DBus::Variant>;
using ManagedObjects = std::map<DBus::Path, std::map<std::string, PropMap>>;

// dbus-cxx's Dispatcher owns a background thread that pumps every
// connection/signal callback created against it; one process-wide instance
// is shared by every Connection/ScanHandle, ref-counted the same way the
// previous gattlib backend's Mainloop singleton was (and for the same
// reason: avoid a dispatch thread per BlueZGattSensor instance).
class Bus {
 public:
  static Bus& Instance() {
    static Bus instance;
    return instance;
  }

  std::shared_ptr<DBus::Connection> Acquire() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (ref_count_++ == 0) {
      // dbus-cxx/libdbus's compiled-in default system bus address doesn't
      // necessarily match where this device's own dbus-daemon actually
      // listens: the Kobo has no /run/dbus at all, only
      // /var/run/dbus/system_bus_socket -- confirmed on a real device (the
      // previous gattlib backend needed the exact same fix for the same
      // reason, see its Mainloop::Acquire()). setenv(..., 0) leaves a
      // deliberate external override in place.
      setenv("DBUS_SYSTEM_BUS_ADDRESS", "unix:path=/var/run/dbus/system_bus_socket", 0);
      dispatcher_ = DBus::StandaloneDispatcher::create();
      connection_ = dispatcher_->create_connection(DBus::BusType::SYSTEM);
    }
    return connection_;
  }

  void Release() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (--ref_count_ == 0) {
      connection_.reset();
      dispatcher_.reset();
    }
  }

 private:
  std::mutex mutex_;
  int ref_count_ = 0;
  std::shared_ptr<DBus::Dispatcher> dispatcher_;
  std::shared_ptr<DBus::Connection> connection_;
};

std::string DevicePathForAddress(const std::string& address) {
  std::string dev_id = address;
  for (char& c : dev_id) {
    c = (c == ':') ? '_' : static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  }
  return std::string(kAdapterPath) + "/dev_" + dev_id;
}

bool PathIsUnderDevice(const std::string& path, const std::string& device_path) {
  return path.size() > device_path.size() &&
         path.compare(0, device_path.size(), device_path) == 0 &&
         path[device_path.size()] == '/';
}

// ---- low-level blocking D-Bus call helpers. Must never be called from the
// dispatcher thread itself (i.e. from inside a signal callback) -- that
// thread is what delivers the reply, so blocking on it there deadlocks.
// Every caller of these below runs on its own std::thread instead. ----

// dbus-cxx's own default blocking-call timeout is 20s (Connection::
// send_with_reply_blocking's timeout_milliseconds=-1 case) -- long enough
// for property/method calls that resolve locally within bluetoothd, but not
// always for Device1.Connect() specifically: BlueZ only replies to that
// call once the real over-the-air LE connection (and, for a not-yet-paired
// device, the connection-parameter negotiation that follows) has actually
// completed, which can take longer than 20s under a marginal signal --
// confirmed against a real device (org.freedesktop.DBus.Error.NoReply at
// ~20s while BlueZ kept the connection attempt itself alive in the
// background, later surfacing as "Operation already in progress"). Give
// that one call more room; every other call here is fine with the default.
constexpr int kDefaultTimeoutMs = -1;
constexpr int kConnectTimeoutMs = 30000;

std::shared_ptr<DBus::ReturnMessage> CallBlocking(
    const std::shared_ptr<DBus::Connection>& conn, const std::string& path,
    const std::string& iface, const std::string& method,
    int timeout_ms = kDefaultTimeoutMs) {
  auto msg = DBus::CallMessage::create(kBluezService, path, iface, method);
  return conn->send_with_reply_blocking(msg, timeout_ms);
}

DBus::Variant GetProperty(const std::shared_ptr<DBus::Connection>& conn,
                          const std::string& path, const std::string& iface,
                          const std::string& name) {
  auto msg = DBus::CallMessage::create(kBluezService, path, kPropsIface, "Get");
  msg << iface << name;
  auto reply = conn->send_with_reply_blocking(msg);
  DBus::Variant v;
  reply >> v;
  return v;
}

ManagedObjects GetManagedObjects(const std::shared_ptr<DBus::Connection>& conn) {
  auto msg = DBus::CallMessage::create(kBluezService, "/", kObjMgrIface, "GetManagedObjects");
  auto reply = conn->send_with_reply_blocking(msg);
  ManagedObjects objects;
  reply >> objects;
  return objects;
}

std::vector<uint8_t> ReadValue(const std::shared_ptr<DBus::Connection>& conn,
                               const std::string& char_path) {
  auto msg = DBus::CallMessage::create(kBluezService, char_path, kChar1Iface, "ReadValue");
  PropMap options;
  msg << options;
  auto reply = conn->send_with_reply_blocking(msg);
  std::vector<uint8_t> data;
  reply >> data;
  return data;
}

void WriteValue(const std::shared_ptr<DBus::Connection>& conn,
                const std::string& char_path, const void* data, size_t size) {
  auto msg = DBus::CallMessage::create(kBluezService, char_path, kChar1Iface, "WriteValue");
  const auto* bytes = static_cast<const uint8_t*>(data);
  std::vector<uint8_t> value(bytes, bytes + size);
  PropMap options;
  msg << value << options;
  conn->send_with_reply_blocking(msg);
}

// Returns {fd, mtu}; fd is null on failure.
std::pair<std::shared_ptr<DBus::FileDescriptor>, uint16_t> AcquireWrite(
    const std::shared_ptr<DBus::Connection>& conn, const std::string& char_path) {
  auto msg = DBus::CallMessage::create(kBluezService, char_path, kChar1Iface, "AcquireWrite");
  PropMap options;
  msg << options;
  auto reply = conn->send_with_reply_blocking(msg);
  std::shared_ptr<DBus::FileDescriptor> fd;
  uint16_t mtu = 0;
  reply >> fd >> mtu;
  return {fd, mtu};
}

// True if BlueZ reports this device as BR/EDR-capable (i.e. it should use
// BT_SPP:, not BLE:). Checked via the device's "Class" property rather than
// "UUIDs": UUIDs stays empty ("ServicesResolved": false) until BlueZ has
// actually connected and done an SDP lookup, which hasn't happened yet for a
// device that's merely been seen during a scan. "Class" (Class of Device) is
// a BR/EDR-only concept BlueZ only ever sets for BR/EDR-capable devices; a
// pure-LE peripheral simply has no Class property at all, cached or not.
// Same semantics as the previous gattlib backend's IsClassicCapable().
bool IsClassicCapable(const std::shared_ptr<DBus::Connection>& conn, const std::string& device_path) {
  try {
    GetProperty(conn, device_path, kDevice1Iface, "Class");
    return true;
  } catch (const DBus::Error&) {
    return false;
  }
}

} // namespace

struct Connection {
  Connection() = delete;
  Connection(const Connection&) = delete;
  Connection& operator=(const Connection&) = delete;

  Connection(std::string address, const Callbacks& callbacks)
      : address(std::move(address)), callbacks(callbacks) {
    device_path = DevicePathForAddress(this->address);
  }

  std::string address;
  std::string device_path;
  Callbacks callbacks;
  std::shared_ptr<DBus::Connection> conn;

  // Guards everything below, and serializes Disconnect() against any
  // in-flight signal callback for this Connection -- mirrors the previous
  // gattlib backend's Connection::callback_mutex, same reasoning (a
  // just-delivered signal callback and Disconnect() tearing this Connection
  // down must never run concurrently). Recursive because a callback that
  // triggers a reconnect attempt re-enters via ConnectAsync() while already
  // holding this from the disconnect handler.
  std::recursive_mutex callback_mutex;
  std::atomic<bool> shutting_down{false};
  std::atomic<bool> ever_connected{false};

  // Characteristic UUID -> (object path, owning service UUID), filled in
  // during discovery.
  struct CharInfo {
    std::string path;
    uuid_t service;
  };
  std::unordered_map<uuid_t, CharInfo, uuid_hash> char_info;

  bool has_write_char = false;
  std::string write_char_path;
  std::shared_ptr<DBus::FileDescriptor> write_fd;
  uint16_t write_mtu = 0;

  // Kept alive for the lifetime of the connection; destroying these
  // unsubscribes. device_props_signal watches Connected/ServicesResolved on
  // the device itself; char_props_signals watches Value on every
  // subscribed characteristic.
  using PropsChangedSignal = DBus::SignalProxy<void(std::string, PropMap, std::vector<std::string>)>;
  std::shared_ptr<PropsChangedSignal> device_props_signal;
  std::vector<std::shared_ptr<PropsChangedSignal>> char_props_signals;

  std::thread worker; // whichever async operation (connect, reconnect) is in flight
};

namespace {

void TeardownCharSignals(Connection* self) {
  self->char_props_signals.clear();
  self->char_info.clear();
  self->has_write_char = false;
  self->write_fd.reset();
  self->write_mtu = 0;
}

void SubscribeCharacteristicNotify(Connection* self, const std::string& char_path,
                                   const uuid_t& char_uuid, const uuid_t& service_uuid) {
  auto rule = DBus::MatchRuleBuilder::create()
                  .set_path(char_path)
                  .set_interface(kPropsIface)
                  .set_member(kPropsChangedMember)
                  .as_signal_match();
  auto signal = self->conn->create_free_signal_proxy<void(std::string, PropMap, std::vector<std::string>)>(
      rule, DBus::ThreadForCalling::DispatcherThread);

  signal->connect([self, char_uuid, service_uuid](std::string /*iface*/, PropMap changed,
                                                   std::vector<std::string> /*invalidated*/) {
    // on_characteristic_changed() must never run while callback_mutex is
    // held: GattSensor::OnCharacteristicChanged() only takes GattSensor's
    // own mutex, but that mutex is also held by WriteData()/
    // DoWriteGattCharacteristic()/DoReadGattCharacteristic() while THEY wait
    // on callback_mutex -- holding both here would be a lock-order
    // inversion against those. Snapshot the payload under the lock, then
    // call out unlocked.
    std::vector<uint8_t> data;
    {
      std::lock_guard<std::recursive_mutex> lock(self->callback_mutex);
      if (self->shutting_down.load()) {
        return;
      }
      auto it = changed.find("Value");
      if (it == changed.end()) {
        return;
      }
      data = it->second.template to_vector<uint8_t>();
    }
    self->callbacks.on_characteristic_changed(self->callbacks.self, service_uuid, char_uuid,
                                              data.data(), data.size());
  });

  // Re-check shutting_down here (not just inside DiscoverAndSubscribe's own
  // brief lock windows, see below): this function runs unlocked between its
  // creation of `signal` above and this point, so a concurrent Disconnect()
  // may have already torn down char_props_signals by the time we get here.
  // If so, just let `signal` go out of scope unsubscribed rather than
  // resurrecting it into a connection that's going away.
  std::lock_guard<std::recursive_mutex> lock(self->callback_mutex);
  if (self->shutting_down.load()) {
    return;
  }
  self->char_props_signals.push_back(std::move(signal));
}

// Runs entirely on a background thread (spawned by whoever calls it): reads
// the GATT object tree via GetManagedObjects(), matches every
// GattCharacteristic1 under this device's path against its owning
// GattService1, and subscribes to notifications / does one-shot reads per
// should_enable_notification(), exactly like the previous backend's
// DiscoverAndSubscribe().
//
// callback_mutex is held only for brief data-structure mutations below, NEVER
// across a blocking D-Bus call or a callbacks.* invocation. This loop can run
// 20+ StartNotify/ReadValue round trips (40+ seconds total, confirmed against
// a real 21-characteristic device) -- holding the lock for all of that once
// starved Disconnect()'s "fast" synchronous part (same lock) long enough for
// the Kobo's own watchdog to SIGKILL the whole app while it looked frozen.
bool DiscoverAndSubscribe(Connection* self) {
  if (self->shutting_down.load()) {
    return false;
  }

  ManagedObjects objects = GetManagedObjects(self->conn);

  if (self->shutting_down.load()) {
    return false;
  }

  // service object path -> service UUID
  std::unordered_map<std::string, uuid_t> service_uuid_by_path;
  for (auto& [path, ifaces] : objects) {
    if (!PathIsUnderDevice(path, self->device_path)) {
      continue;
    }
    auto it = ifaces.find(kService1Iface);
    if (it == ifaces.end()) {
      continue;
    }
    auto uuid_it = it->second.find("UUID");
    if (uuid_it == it->second.end()) {
      continue;
    }
    service_uuid_by_path[path] = uuid_t(uuid_it->second.to_string());
  }

  struct CharPlan {
    std::string path;
    uuid_t char_uuid;
    uuid_t service_uuid;
    bool can_notify;
    bool can_read;
    bool can_write;
  };
  std::vector<CharPlan> plans;

  for (auto& [path, ifaces] : objects) {
    if (!PathIsUnderDevice(path, self->device_path)) {
      continue;
    }
    auto char_it = ifaces.find(kChar1Iface);
    if (char_it == ifaces.end()) {
      continue;
    }
    PropMap& props = char_it->second;

    auto uuid_it = props.find("UUID");
    auto service_it = props.find("Service");
    auto flags_it = props.find("Flags");
    if (uuid_it == props.end() || service_it == props.end() || flags_it == props.end()) {
      continue;
    }

    const uuid_t char_uuid(uuid_it->second.to_string());
    const std::string service_path = service_it->second.to_path();
    auto service_uuid_it = service_uuid_by_path.find(service_path);
    const uuid_t service_uuid = (service_uuid_it != service_uuid_by_path.end())
                                     ? service_uuid_it->second : uuid_t(0, 0);
    const std::vector<std::string> flags = flags_it->second.to_vector<std::string>();
    const bool can_notify = std::find(flags.begin(), flags.end(), "notify") != flags.end() ||
                            std::find(flags.begin(), flags.end(), "indicate") != flags.end();
    const bool can_read = std::find(flags.begin(), flags.end(), "read") != flags.end();
    const bool can_write = std::find(flags.begin(), flags.end(), "write") != flags.end() ||
                           std::find(flags.begin(), flags.end(), "write-without-response") != flags.end();

    plans.push_back(CharPlan{path, char_uuid, service_uuid, can_notify, can_read, can_write});
  }

  std::string write_char_path;
  bool has_write_char = false;
  {
    std::lock_guard<std::recursive_mutex> lock(self->callback_mutex);
    if (self->shutting_down.load()) {
      return false;
    }
    TeardownCharSignals(self);
    for (const auto& plan : plans) {
      self->char_info[plan.char_uuid] = Connection::CharInfo{plan.path, plan.service_uuid};
      if (plan.char_uuid == HM10_RX_TX_CHARACTERISTIC && plan.can_write) {
        self->write_char_path = plan.path;
        self->has_write_char = true;
      }
    }
    write_char_path = self->write_char_path;
    has_write_char = self->has_write_char;
  }

  for (const auto& plan : plans) {
    if (self->shutting_down.load()) {
      return false;
    }

    // should_enable_notification() can reach GattSensor::DoEnableNotification(),
    // which takes CritSec_Comm for any characteristic not in the fixed
    // service_table() -- must run unlocked here for the same reason as the
    // notification callback above (a device thread can be holding
    // CritSec_Comm while it waits on callback_mutex via Write()).
    if (!self->callbacks.should_enable_notification(self->callbacks.self, plan.service_uuid, plan.char_uuid)) {
      continue;
    }

    if (plan.can_notify) {
      try {
        CallBlocking(self->conn, plan.path, kChar1Iface, "StartNotify");
        SubscribeCharacteristicNotify(self, plan.path, plan.char_uuid, plan.service_uuid);
      } catch (const DBus::Error&) {
        // Best-effort, matches previous backend's silent-skip-on-failure.
      }
    } else if (plan.can_read) {
      try {
        std::vector<uint8_t> data = ReadValue(self->conn, plan.path);
        self->callbacks.on_characteristic_changed(self->callbacks.self, plan.service_uuid, plan.char_uuid,
                                                  data.data(), data.size());
      } catch (const DBus::Error&) {
      }
    }
  }

  if (has_write_char && !self->shutting_down.load()) {
    // Best-effort MTU-negotiated write stream; Write() falls back to an
    // unchunked WriteValue() if this failed.
    try {
      auto [fd, mtu] = AcquireWrite(self->conn, write_char_path);
      std::lock_guard<std::recursive_mutex> lock(self->callback_mutex);
      if (!self->shutting_down.load()) {
        self->write_fd = fd;
        self->write_mtu = mtu;
      }
    } catch (const DBus::Error&) {
    }
  }

  return true;
}

void ConnectAsync(Connection* self); // fwd decl, for the reconnect call below

// Guards its data-structure/self->worker mutations with callback_mutex, but
// never calls out to callbacks.on_connected/on_disconnected while holding it
// -- those can reach GattSensor's own mutex (via PortStateChanged(), called
// from BlueZGattSensor::OnConnected/OnDisconnected), which is also held by
// WriteData()/DoWriteGattCharacteristic()/DoReadGattCharacteristic() while
// THEY wait on callback_mutex. Calling out under the lock here would be a
// lock-order inversion against those (real deadlock risk, not just a stall).
void HandleDeviceProperties(Connection* self, PropMap changed) {
  auto connected_it = changed.find("Connected");
  if (connected_it != changed.end() && !connected_it->second.to_bool()) {
    bool was_connected;
    {
      std::lock_guard<std::recursive_mutex> lock(self->callback_mutex);
      if (self->shutting_down.load()) {
        return;
      }
      was_connected = self->ever_connected.exchange(false);
      TeardownCharSignals(self);
      // Single reconnect attempt, mirroring the previous backend's retry
      // policy (and the Android backend's gatt.connect() retry) on an
      // unsolicited disconnect.
      if (self->worker.joinable()) {
        self->worker.detach(); // the connect attempt that got us here is done
      }
      self->worker = std::thread(ConnectAsync, self);
    }
    if (was_connected) {
      self->callbacks.on_disconnected(self->callbacks.self);
    }
    return;
  }

  auto resolved_it = changed.find("ServicesResolved");
  if (resolved_it != changed.end() && resolved_it->second.to_bool()) {
    // DiscoverAndSubscribe() makes several blocking D-Bus calls; this
    // handler runs on the dispatcher thread itself (see
    // SubscribeDeviceProperties()), and that thread is what delivers the
    // reply to any blocking call it makes -- calling it inline here would
    // deadlock. Run it on self->worker instead, same as the reconnect
    // branch above, so Disconnect()'s worker.join() still waits for it.
    std::lock_guard<std::recursive_mutex> lock(self->callback_mutex);
    if (self->shutting_down.load()) {
      return;
    }
    if (self->worker.joinable()) {
      self->worker.detach();
    }
    self->worker = std::thread([self] {
      const bool ok = DiscoverAndSubscribe(self);
      self->ever_connected = ok;
      self->callbacks.on_connected(self->callbacks.self, ok);
    });
  }
}

void SubscribeDeviceProperties(Connection* self) {
  auto rule = DBus::MatchRuleBuilder::create()
                  .set_path(self->device_path)
                  .set_interface(kPropsIface)
                  .set_member(kPropsChangedMember)
                  .as_signal_match();
  self->device_props_signal = self->conn->create_free_signal_proxy<void(std::string, PropMap, std::vector<std::string>)>(
      rule, DBus::ThreadForCalling::DispatcherThread);
  self->device_props_signal->connect(
      [self](std::string /*iface*/, PropMap changed, std::vector<std::string> /*invalidated*/) {
        HandleDeviceProperties(self, std::move(changed));
      });
}

// Runs on its own thread (self->worker): (re)establishes the connection --
// rescans for the device's D-Bus object (BlueZ drops an unpaired/"temporary"
// device's Device1 object once discovery stops, so a stale cached object
// path can't be assumed to still exist -- same reasoning, and same fix, as
// the previous backend's "always rescan immediately before connecting"),
// then calls Device1.Connect(). Service discovery itself happens later,
// driven by HandleDeviceProperties() once ServicesResolved flips true.
void ConnectAsync(Connection* self) {
  bool found = false;
  // Declared before added_signal below so they outlive it: C++ destroys
  // automatic variables in reverse declaration order, and added_signal's
  // callback (running on the dispatcher thread until added_signal itself
  // is torn down) captures these by reference.
  std::mutex found_mutex;
  std::condition_variable found_cv;
  try {
    // Already known from a previous scan/pairing?
    ManagedObjects objects = GetManagedObjects(self->conn);
    found = objects.count(self->device_path) != 0;

    if (!found && !self->shutting_down.load()) {
      auto rule = DBus::MatchRuleBuilder::create()
                      .set_path("/")
                      .set_interface(kObjMgrIface)
                      .set_member("InterfacesAdded")
                      .as_signal_match();
      auto added_signal = self->conn->create_free_signal_proxy<void(DBus::Path, std::map<std::string, PropMap>)>(
          rule, DBus::ThreadForCalling::DispatcherThread);

      added_signal->connect([&](DBus::Path path, std::map<std::string, PropMap> ifaces) {
        if (path == self->device_path && ifaces.count(kDevice1Iface)) {
          std::lock_guard<std::mutex> lock(found_mutex);
          found = true;
          found_cv.notify_all();
        }
      });

      CallBlocking(self->conn, kAdapterPath, kAdapter1Iface, "StartDiscovery");
      {
        std::unique_lock<std::mutex> lock(found_mutex);
        found_cv.wait_for(lock, std::chrono::seconds(10), [&] {
          return found || self->shutting_down.load();
        });
      }
      try {
        CallBlocking(self->conn, kAdapterPath, kAdapter1Iface, "StopDiscovery");
      } catch (const DBus::Error&) {
      }
    }
  } catch (const DBus::Error&) {
    found = false;
  }

  if (self->shutting_down.load()) {
    return;
  }

  if (!found) {
    self->callbacks.on_connected(self->callbacks.self, false);
    return;
  }

  try {
    CallBlocking(self->conn, self->device_path, kDevice1Iface, "Connect", kConnectTimeoutMs);
  } catch (const DBus::Error&) {
    if (!self->shutting_down.load()) {
      self->callbacks.on_connected(self->callbacks.self, false);
    }
    return;
  }

  if (self->shutting_down.load()) {
    return;
  }

  // Connect() may already imply ServicesResolved by the time it returns;
  // HandleDeviceProperties() (already subscribed before this call) handles
  // the case where it resolves slightly later instead.
  try {
    DBus::Variant resolved = GetProperty(self->conn, self->device_path, kDevice1Iface, "ServicesResolved");
    if (resolved.to_bool()) {
      const bool ok = DiscoverAndSubscribe(self);
      self->ever_connected = ok;
      self->callbacks.on_connected(self->callbacks.self, ok);
    }
  } catch (const DBus::Error&) {
  }
}

} // namespace

struct ScanHandle {
  std::shared_ptr<DBus::Connection> conn;
  void (*callback)(void*, const char*, const char*, bool) = nullptr;
  void* user_data = nullptr;
  std::shared_ptr<DBus::SignalProxy<void(DBus::Path, std::map<std::string, PropMap>)>> added_signal;
  std::mutex seen_mutex;
  std::unordered_map<std::string, bool> seen; // address -> is_classic_spp
};

namespace {

std::string AddressFromDevicePath(const std::string& path) {
  auto pos = path.rfind("/dev_");
  if (pos == std::string::npos) {
    return {};
  }
  std::string dev_id = path.substr(pos + 5);
  for (char& c : dev_id) {
    if (c == '_') c = ':';
  }
  return dev_id;
}

void ReportDevice(ScanHandle* handle, const std::string& path, const PropMap& device_props) {
  const std::string address = AddressFromDevicePath(path);
  if (address.empty()) {
    return;
  }
  std::string name;
  auto name_it = device_props.find("Name");
  if (name_it != device_props.end()) {
    name = name_it->second.to_string();
  }

  {
    std::lock_guard<std::mutex> lock(handle->seen_mutex);
    auto it = handle->seen.find(address);
    if (it != handle->seen.end()) {
      handle->callback(handle->user_data, address.c_str(), name.c_str(), it->second);
      return;
    }
  }

  // First time seeing this address this scan: IsClassicCapable() is a
  // synchronous D-Bus round trip and must never run on the dispatcher
  // thread (deadlocks waiting for its own reply) -- check it off-thread,
  // then cache the result for next time, same as the previous backend. The
  // catch-all guards against an uncaught exception on this detached thread
  // calling std::terminate() and taking down the whole process.
  std::thread([handle, path, address, name] {
    bool is_classic_spp = false;
    try {
      is_classic_spp = IsClassicCapable(handle->conn, path);
    } catch (...) {
      return;
    }
    {
      std::lock_guard<std::mutex> lock(handle->seen_mutex);
      handle->seen[address] = is_classic_spp;
    }
    handle->callback(handle->user_data, address.c_str(), name.c_str(), is_classic_spp);
  }).detach();
}

} // namespace

ScanHandle* StartScan(void (*callback)(void*, const char*, const char*, bool), void* user_data) {
  auto* handle = new ScanHandle();
  handle->callback = callback;
  handle->user_data = user_data;
  handle->conn = Bus::Instance().Acquire();

  try {
    ManagedObjects objects = GetManagedObjects(handle->conn);
    for (const auto& [path, ifaces] : objects) {
      auto it = ifaces.find(kDevice1Iface);
      if (it != ifaces.end()) {
        ReportDevice(handle, path, it->second);
      }
    }

    auto rule = DBus::MatchRuleBuilder::create()
                    .set_path("/")
                    .set_interface(kObjMgrIface)
                    .set_member("InterfacesAdded")
                    .as_signal_match();
    handle->added_signal = handle->conn->create_free_signal_proxy<void(DBus::Path, std::map<std::string, PropMap>)>(
        rule, DBus::ThreadForCalling::DispatcherThread);
    handle->added_signal->connect([handle](DBus::Path path, std::map<std::string, PropMap> ifaces) {
      auto it = ifaces.find(kDevice1Iface);
      if (it != ifaces.end()) {
        ReportDevice(handle, path, it->second);
      }
    });

    CallBlocking(handle->conn, kAdapterPath, kAdapter1Iface, "StartDiscovery");
  } catch (const DBus::Error&) {
    Bus::Instance().Release();
    delete handle;
    return nullptr;
  }

  return handle;
}

void StopScan(ScanHandle* handle) {
  if (!handle) {
    return;
  }
  handle->added_signal.reset();
  try {
    CallBlocking(handle->conn, kAdapterPath, kAdapter1Iface, "StopDiscovery");
  } catch (const DBus::Error&) {
  }
  Bus::Instance().Release();
  delete handle;
}

Connection* Connect(const char* address, const Callbacks& callbacks) {
  auto* self = new Connection(address, callbacks);
  self->conn = Bus::Instance().Acquire();

  SubscribeDeviceProperties(self);
  self->worker = std::thread(ConnectAsync, self);

  return self;
}

void Disconnect(Connection* connection) {
  if (!connection) {
    return;
  }

  {
    // Taking the lock here (before setting shutting_down) closes the gap
    // where a callback already read shutting_down as false and is about to
    // act on state that's about to go away -- it now either hasn't started
    // yet (and will see it true once it does get the lock) or is already
    // inside its own critical section, in which case this block waits for
    // it to finish first. device_props_signal/char_info/etc. are also torn
    // down inside this same critical section (not after it): both
    // DiscoverAndSubscribe() and SubscribeCharacteristicNotify() mutate
    // them under this same lock from another thread, so tearing them down
    // unlocked here would race with that. Mirrors the previous backend's
    // Disconnect(). This part is fast (no network round trip beyond
    // unsubscribing signals) and runs synchronously on the caller's thread.
    std::lock_guard<std::recursive_mutex> lock(connection->callback_mutex);
    connection->shutting_down = true;
    connection->device_props_signal.reset();
    TeardownCharSignals(connection);
  }

  // Everything else below -- joining self->worker (which can be mid-way
  // through a slow real connect attempt: up to kConnectTimeoutMs, plus the
  // rescan wait, plus per-characteristic discovery round trips) and the
  // final Device1.Disconnect() call -- must NOT block the calling thread.
  // BlueZGattSensor::Disconnect() already cleared its own `connection`
  // pointer before calling this, so LK8000's view of the port is already
  // consistent the instant this function returns; the caller does not need
  // `connection` to be fully torn down yet. This matters because LK8000
  // calls this synchronously from ProcessTimer() -> RestartCommPorts(),
  // which runs on the main UI thread at 2Hz -- blocking here for the length
  // of a slow connect attempt froze the whole UI (confirmed on a real
  // device: the app appeared hung for the duration of a stuck reconnect).
  std::thread([connection] {
    if (connection->worker.joinable()) {
      connection->worker.join();
    }

    // Wait for any HandleDeviceProperties() callback that was already in
    // flight when shutting_down was set above to finish.
    { std::lock_guard<std::recursive_mutex> lock(connection->callback_mutex); }

    try {
      CallBlocking(connection->conn, connection->device_path, kDevice1Iface, "Disconnect");
    } catch (const DBus::Error&) {
    }

    connection->conn.reset();
    Bus::Instance().Release();
    delete connection;
  }).detach();
}

bool Write(Connection* connection, const void* data, size_t size) {
  if (!connection || !connection->ever_connected.load() || !connection->has_write_char) {
    return false;
  }
  // char_info/has_write_char/write_char_path/write_fd/write_mtu are also
  // written (under callback_mutex) from DiscoverAndSubscribe(), which runs
  // on a different thread than this is called from -- snapshot what's
  // needed under the same lock, then do the actual write outside it.
  std::shared_ptr<DBus::FileDescriptor> write_fd;
  uint16_t write_mtu;
  std::string write_char_path;
  {
    std::lock_guard<std::recursive_mutex> lock(connection->callback_mutex);
    if (!connection->has_write_char) {
      return false;
    }
    write_fd = connection->write_fd;
    write_mtu = connection->write_mtu;
    write_char_path = connection->write_char_path;
  }
  if (write_fd) {
    const auto* bytes = static_cast<const uint8_t*>(data);
    size_t sent = 0;
    const uint16_t chunk = write_mtu > 0 ? write_mtu : static_cast<uint16_t>(size);
    while (sent < size) {
      const size_t n = std::min<size_t>(chunk, size - sent);
      if (write(write_fd->descriptor(), bytes + sent, n) < 0) {
        return false;
      }
      sent += n;
    }
    return true;
  }
  try {
    WriteValue(connection->conn, write_char_path, data, size);
    return true;
  } catch (const DBus::Error&) {
    return false;
  }
}

bool WriteCharacteristic(Connection* connection, const uuid_t& characteristic,
                         const void* data, size_t size) {
  if (!connection || !connection->ever_connected.load()) {
    return false;
  }
  std::string path;
  {
    std::lock_guard<std::recursive_mutex> lock(connection->callback_mutex);
    auto it = connection->char_info.find(characteristic);
    if (it == connection->char_info.end()) {
      return false;
    }
    path = it->second.path;
  }
  try {
    WriteValue(connection->conn, path, data, size);
    return true;
  } catch (const DBus::Error&) {
    return false;
  }
}

void ReadCharacteristic(Connection* connection, const uuid_t& service, const uuid_t& characteristic) {
  if (!connection || !connection->ever_connected.load()) {
    return;
  }
  std::string path;
  {
    std::lock_guard<std::recursive_mutex> lock(connection->callback_mutex);
    auto it = connection->char_info.find(characteristic);
    if (it == connection->char_info.end()) {
      return;
    }
    path = it->second.path;
  }
  // Run the blocking D-Bus round-trip on its own thread so callers (device
  // driver threads holding CritSec_Comm) never stall on it.
  std::thread([connection, service, characteristic, path] {
    try {
      std::vector<uint8_t> data = ReadValue(connection->conn, path);
      connection->callbacks.on_characteristic_changed(connection->callbacks.self, service, characteristic,
                                                       data.data(), data.size());
    } catch (const DBus::Error&) {
    }
  }).detach();
}

} // namespace bluez_dbus_backend
