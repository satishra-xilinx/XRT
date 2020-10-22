/*
 * Copyright (C) 2020, Xilinx Inc - All rights reserved
 * Xilinx Runtime (XRT) Experimental APIs
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You may
 * not use this file except in compliance with the License. A copy of the
 * License is located at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

#ifndef _XRT_KERNEL_H_
#define _XRT_KERNEL_H_

#include "xrt.h"
#include "ert.h"
#include "experimental/xrt_uuid.h"
#include "experimental/xrt_bo.h"
#include "experimental/xrt_device.h"

#ifdef __cplusplus
# include "experimental/xrt_enqueue.h"
# include <memory>
# include <vector>
# include <functional>
# include <chrono>
# include <cstdint>
#endif

/**
 * typedef xrtKernelHandle - opaque kernel handle
 *
 * A kernel handle is obtained by opening a kernel.  Clients
 * pass this kernel handle to APIs that operate on a kernel.
 */
typedef void * xrtKernelHandle;

/**
 * typedef xrtRunHandle - opaque handle to a specific kernel run
 *
 * A run handle is obtained by running a kernel.  Clients
 * use a run handle to check or wait for kernel completion.
 */
typedef void * xrtRunHandle;

#ifdef __cplusplus

namespace xrt {

class kernel;
class event_impl;

/*!
 * @class run 
 *
 * @brief 
 * xrt::run represents one execution of a kernel
 *
 * @details
 * The run handle can be explicitly constructed from a kernel object
 * or implicitly constructed from starting a kernel execution.
 *
 * A run handle can be re-used to execute the same kernel again.
 */
class run_impl;
class run
{
 public:
  /**
   * run() - Construct empty run object
   *
   * Can be used as lvalue in assignment.
   */
  run()
  {}

  /**
   * run() - Construct run object from a kernel object
   *
   * @param krnl: Kernel object representing the kernel to execute
   */
  XCL_DRIVER_DLLESPEC
  explicit
  run(const kernel& krnl);

  /**
   * start() - Start execution of a run.
   *
   * This function is asynchronous, run status must be expclicit checked
   * or ``wait()`` must be used to wait for the run to complete.
   */
  XCL_DRIVER_DLLESPEC
  void
  start();

  /**
   * wait() - Wait for a run to complete execution
   *
   * @param timeout  
   *  Timeout for wait (default block till run completes)
   * @return 
   *  Command state upon return of wait
   *
   * The default timeout of 0ms indicates blocking until run completes.
   *
   * The current thread will block until the run completes or timeout
   * expires. Completion does not guarantee success, the run status
   * should be checked by using ``state``.
   */
  XCL_DRIVER_DLLESPEC
  ert_cmd_state
  wait(const std::chrono::milliseconds& timeout = std::chrono::milliseconds{0}) const;

  /**
   * wait() - Wait for specified milliseconds for run to complete
   *
   * @param timeout_ms
   *  Timeout in milliseconds
   * @return
   *  Command state upon return of wait
   *
   * The default timeout of 0ms indicates blocking until run completes.
   *
   * The current thread will block until the run completes or timeout
   * expires. Completion does not guarantee success, the run status
   * should be checked by using ``state``.
   */
  ert_cmd_state
  wait(unsigned int timeout_ms) const
  {
    return wait(timeout_ms * std::chrono::milliseconds{1});
  }

  /**
   * state() - Check the current state of a run object
   *
   * @return 
   *  Current state of this run object
   *
   * The state values are defined in ``include/ert.h``
   */
  XCL_DRIVER_DLLESPEC
  ert_cmd_state
  state() const;

  /**
   * add_callback() - Add a callback function for run state
   *
   * @param state       State to invoke callback on
   * @param callback    Callback function 
   * @param data        User data to pass to callback function
   *
   * The function is called when the run object changes state to
   * argument state or any error state.  Only
   * ``ERT_CMD_STATE_COMPLETED`` is supported currently.
   *
   * Any number of callbacks are supported.
   */
  XCL_DRIVER_DLLESPEC
  void
  add_callback(ert_cmd_state state,
               std::function<void(const run&, ert_cmd_state, void*)> callback,
               void* data);


  /**
   * set_event() - Add event for enqueued operations
   *
   * @param event    
   *   Opaque implementation object
   *
   * This function is used when a run object is enqueued in an event
   * graph.  The event must be notified upon completion of the run.
   */
  XCL_DRIVER_DLLESPEC
  void
  set_event(const std::shared_ptr<event_impl>& event) const;

  /**
   * operator bool() - Check if run handle is valid
   *
   * @return 
   *   True if run is associated with kernel object, false otherwise
   */
  explicit 
  operator bool() const
  {
    return handle != nullptr;
  }

  /**
   * set_arg() - Set a specific kernel scalar argument for this run
   *
   * @param index
   *  Index of kernel argument to update
   * @param arg        
   *  The scalar argument value to set.
   * 
   * Use this API to explicit set or change a kernel argument prior
   * to starting kernel execution.  After setting arguments, the
   * kernel can be started using ``start()`` on the run object.
   *
   * See also ``operator()`` to set all arguments and start kernel.
   */
  template <typename ArgType>
  void
  set_arg(int index, ArgType&& arg)
  {
    set_arg_at_index(index, get_arg_value(std::forward<ArgType>(arg)));
  }

  /**
   * set_arg() - Set a specific kernel global argument for a run
   *
   * @param index
   *  Index of kernel argument to set
   * @param boh
   *  The global buffer argument value to set (lvalue).
   * 
   * Use this API to explicit set or change a kernel argument prior
   * to starting kernel execution.  After setting arguments, the
   * kernel can be started using ``start()`` on the run object.
   *
   * See also ``operator()`` to set all arguments and start kernel.
   */
  void
  set_arg(int index, xrt::bo& boh)
  {
    set_arg_at_index(index, boh);
  }

  /**
   * set_arg - xrt::bo variant for const lvalue
   */
  void
  set_arg(int index, const xrt::bo& boh)
  {
    set_arg_at_index(index, boh);
  }

  /**
   * set_arg - xrt::bo variant for rvalue
   */
  void
  set_arg(int index, xrt::bo&& boh)
  {
    set_arg_at_index(index, boh);
  }

  /**
   * udpdate_arg() - Asynchronous update of scalar kernel global argument
   *
   * @param index
   *  Index of kernel argument to update
   * @param arg
   *  The scalar argument value to set.
   * 
   * Use this API to asynchronously update a specific scalar argument
   * of the kernel associated with the run object.
   *
   * This API is only supported on Edge.
   */
  template <typename ArgType>
  void
  update_arg(int index, ArgType&& arg)
  {
    update_arg_at_index(index, get_arg_value(std::forward<ArgType>(arg)));
  }

  /**
   * update_arg() - Asynchronous update of kernel global argument for a run
   *
   * @param index
   *  Index of kernel argument to update
   * @param boh
   *  The global buffer argument value to set.
   * 
   * Use this API to asynchronously update a specific kernel
   * argument of an existing run.  
   *
   * This API is only supported on Edge.
   */
  void
  update_arg(int index, const xrt::bo& boh)
  {
    update_arg_at_index(index, boh);
  }

  /**
   * operator() - Set all kernel arguments and start the run
   *
   * @param args
   *  Kernel arguments
   *
   * Use this API to explicitly set all kernel arguments and 
   * start kernel execution.
   */
  template<typename ...Args>
  void
  operator() (Args&&... args)
  {
    set_arg(0, std::forward<Args>(args)...);
    start();
  }

public:
  std::shared_ptr<run_impl>
  get_handle() const
  {
    return handle;
  }

private:
  std::shared_ptr<run_impl> handle;

  XCL_DRIVER_DLLESPEC
  void
  set_arg_at_index(int index, const std::vector<uint32_t>&);

  XCL_DRIVER_DLLESPEC
  void
  set_arg_at_index(int index, const xrt::bo&);

  XCL_DRIVER_DLLESPEC
  void
  update_arg_at_index(int index, const std::vector<uint32_t>&);

  XCL_DRIVER_DLLESPEC
  void
  update_arg_at_index(int index, const xrt::bo&);
  
  template<typename ArgType>
  std::vector<uint32_t>
  get_arg_value(ArgType&& arg)
  {
    auto words = std::max(sizeof(ArgType), sizeof(uint32_t)) / sizeof(uint32_t);
    return { reinterpret_cast<const uint32_t*>(&arg), reinterpret_cast<const uint32_t*>(&arg) + words };
  }

  template<typename ArgType, typename ...Args>
  void
  set_arg(int argno, ArgType&& arg, Args&&... args)
  {
    set_arg(argno, std::forward<ArgType>(arg));
    set_arg(++argno, std::forward<Args>(args)...);
  }
};
 

/*!
 * @class kernel
 *
 * A kernel object represents a set of instances matching a specified name.
 * The kernel is created by finding matching kernel instances in the 
 * currently loaded xclbin.
 *
 * Most interaction with kernel objects are through \ref xrt::run objects created 
 * from the kernel object to represent an execution of the kernel
 */
class kernel_impl;
class kernel
{
 public:
  /**
   * cu_access_mode - compute unit access mode
   *
   * @var shared
   *  CUs can be shared between processes
   * @var exclusive
   *  CUs are owned exclusively by this process
   */
  enum class cu_access_mode : bool { exclusive = false, shared = true };

  /**
   * kernel() - Constructor from a device and xclbin
   *
   * @param device
   *  Device on which the kernel should execute
   * @param xclbin_id
   *  UUID of the xclbin with the kernel
   * @param name
   *  Name of kernel to construct
   * @param mode
   *  Open the kernel instances with specified access (default shared)
   *
   * The kernel name must uniquely identify compatible kernel
   * instances (compute units).  Optionally specify which kernel
   * instance(s) to open using
   * "kernelname:{instancename1,instancename2,...}" syntax.  The
   * compute units are default opened with shared access, meaning that
   * other kernels and other process will have shared access to same
   * compute units.
   */
  XCL_DRIVER_DLLESPEC
  kernel(const xrt::device& device, const xrt::uuid& xclbin_id, const std::string& name,
         cu_access_mode mode = cu_access_mode::shared);

  /// @cond
  /// Deprecated construtor for exclusive access
  kernel(const xrt::device& device, const xrt::uuid& xclbin_id, const std::string& name, bool ex)
    : kernel(device, xclbin_id, name, ex ? cu_access_mode::exclusive : cu_access_mode::shared)
  {}

  /**
   * Obsoleted construction from xclDeviceHandle
   */
  XCL_DRIVER_DLLESPEC
  kernel(xclDeviceHandle dhdl, const xrt::uuid& xclbin_id, const std::string& name,
         cu_access_mode mode = cu_access_mode::shared);
  /// @endcond

  /**
   * operator() - Invoke the kernel function
   *
   * @param args
   *  Kernel arguments
   * @return 
   *  Run object representing this kernel function invocation
   */
  template<typename ...Args>
  run
  operator() (Args&&... args)
  {
    run r(*this);
    r(std::forward<Args>(args)...);
    return r;
  }

  /**
   * group_id() - Get the memory bank group id of an kernel argument
   *
   * @param argno
   *  The argument index
   * @return
   *  The memory group id to use when allocating buffers (see xrt::bo)
   *
   * The function throws if the group id is ambigious.
   */
  XCL_DRIVER_DLLESPEC
  int
  group_id(int argno) const;

  /**
   * offset() - Get the offset of kernel argument
   *
   * @param argno
   *  The argument index
   * @return
   *  The kernel register offset of the argument with specified index
   *
   * Use with ``read_register()`` and ``write_register()`` if manually
   * reading or writing kernel registers for explicit arguments. 
   */
  XCL_DRIVER_DLLESPEC
  uint32_t
  offset(int argno) const;

  /**
   * write() - Write to the address range of a kernel
   *
   * @param offset
   *  Offset in register space to write to
   * @param data     
   *  Data to write
   *
   * Throws std::out_or_range if offset is outside the
   * kernel address space
   *
   * The kernel must be associated with exactly one kernel instance 
   * (compute unit), which must be opened for exclusive access.
   */
  XCL_DRIVER_DLLESPEC
  void
  write_register(uint32_t offset, uint32_t data);

  /**
   * read() - Read data from kernel address range
   *
   * @param offset  
   *  Offset in register space to read from
   * @return 
   *  Value read from offset
   *
   * Throws std::out_or_range if offset is outside the
   * kernel address space
   *
   * The kernel must be associated with exactly one kernel instance 
   * (compute unit), which must be opened for exclusive access.
   */
  XCL_DRIVER_DLLESPEC
  uint32_t
  read_register(uint32_t offset) const;

public:
  /// @cond
  std::shared_ptr<kernel_impl>
  get_handle() const
  {
    return handle;
  }
  /// @endcond

private:
  std::shared_ptr<kernel_impl> handle;
};

/// @cond
// Specialization from xrt_enqueue.h for run objects, which
// are asynchronous waitable objects.
template <>
struct callable_traits<run>
{
  enum { is_async = true };
};
/// @endcond

} // namespace xrt

/// @cond
extern "C" {
#endif

/**
 * xrtPLKernelOpen() - Open a PL kernel and obtain its handle.
 *
 * @deviceHandle:  Handle to the device with the kernel
 * @xclbinId:      The uuid of the xclbin with the specified kernel.
 * @name:          Name of kernel to open.
 * Return:         Handle representing the opened kernel.
 *
 * The kernel name must uniquely identify compatible kernel instances
 * (compute units).  Optionally specify which kernel instance(s) to
 * open using "kernelname:{instancename1,instancename2,...}" syntax.
 * The compute units are opened with shared access, meaning that 
 * other kernels and other process will have shared access to same
 * compute units.  If exclusive access is needed then open the 
 * kernel using @xrtPLKernelOpenExclusve().
 *
 * An xclbin with the specified kernel must have been loaded prior
 * to calling this function. An XRT_NULL_HANDLE is returned on error
 * and errno is set accordingly.
 *
 * A kernel handle is thread safe and can be shared between threads.
 */
XCL_DRIVER_DLLESPEC
xrtKernelHandle
xrtPLKernelOpen(xrtDeviceHandle deviceHandle, const xuid_t xclbinId, const char *name);

/**
 * xrtPLKernelOpenExclusive() - Open a PL kernel and obtain its handle.
 *
 * @deviceHandle:  Handle to the device with the kernel
 * @xclbinId:      The uuid of the xclbin with the specified kernel.
 * @name:          Name of kernel to open.
 * Return:         Handle representing the opened kernel.
 *
 * Same as @xrtPLKernelOpen(), but opens compute units with exclusive
 * access.  Fails if any compute unit is already opened with either
 * exclusive or shared access.
 */
XCL_DRIVER_DLLESPEC
xrtKernelHandle
xrtPLKernelOpenExclusive(xrtDeviceHandle deviceHandle, const xuid_t xclbinId, const char *name);

/**
 * xrtKernelClose() - Close an opened kernel
 *
 * @kernelHandle: Handle to kernel previously opened with xrtKernelOpen
 * Return:        0 on success, -1 on error
 */
XCL_DRIVER_DLLESPEC
int
xrtKernelClose(xrtKernelHandle kernelHandle);

/**
 * xrtKernelArgGroupId() - Acquire bank group id for kernel argument
 *
 * @kernelHandle: Handle to kernel previously opened with xrtKernelOpen
 * @argno:        Index of kernel argument
 * Return:        Group id or negative error code on error
 *
 * A valid group id is a non-negative integer.  The group id is required
 * when constructing a buffer object.
 *
 * The kernel argument group id is ambigious if kernel has multiple kernel
 * with different connectivity for specified argument.  In this case the
 * API returns error.
 */
XCL_DRIVER_DLLESPEC
int
xrtKernelArgGroupId(xrtKernelHandle kernelHandle, int argno);

/**
 * xrtKernelArgOffset() - Get the offset of kernel argument
 *
 * @khdl:   Handle to kernel previously opened with xrtKernelOpen
 * @argno:  Index of kernel argument
 * Return:  The kernel register offset of the argument with specified index
 *
 * Use with ``xrtKernelReadRegister()`` and ``xrtKernelWriteRegister()`` 
 * if manually reading or writing kernel registers for explicit arguments.
 */
XCL_DRIVER_DLLESPEC
uint32_t
xrtKernelArgOffset(xrtKernelHandle khdl, int argno);

/**
 * xrtKernelReadRegister() - Read data from kernel address range
 *
 * @kernelHandle: Handle to kernel previously opened with xrtKernelOpen
 * @offset:       Offset in register space to read from
 * @datap:        Pointer to location where to write data
 * Return:        0 on success, errcode otherwise
 *
 * The kernel must be associated with exactly one kernel instance 
 * (compute unit), which must be opened for exclusive access.
 */
XCL_DRIVER_DLLESPEC
int
xrtKernelReadRegister(xrtKernelHandle kernelHandle, uint32_t offset, uint32_t* datap);

/**
 * xrtKernelWriteRegister() - Write to the address range of a kernel
 *
 * @kernelHandle: Handle to kernel previously opened with xrtKernelOpen
 * @offset:       Offset in register space to write to
 * @data:         Data to write
 * Return:        0 on success, errcode otherwise
 *
 * The kernel must be associated with exactly one kernel instance 
 * (compute unit), which must be opened for exclusive access.
 */
XCL_DRIVER_DLLESPEC
int
xrtKernelWriteRegister(xrtKernelHandle kernelHandle, uint32_t offset, uint32_t data);

/**
 * xrtKernelRun() - Start a kernel execution
 *
 * @kernelHandle: Handle to the kernel to run
 * @...:          Kernel arguments
 * Return:        Run handle which must be closed with xrtRunClose()
 *
 * A run handle is specific to one execution of a kernel.  Once
 * execution completes, the run handle can be re-used to execute the
 * same kernel again.  When no longer needed, then run handle must be
 * closed with xrtRunClose().
 */
XCL_DRIVER_DLLESPEC
xrtRunHandle
xrtKernelRun(xrtKernelHandle kernelHandle, ...);

/**
 * xrtRunOpen() - Open a new run handle for a kernel without starting kernel
 *
 * @kernelHandle: Handle to the kernel to associate the run handle with
 * Return:        Run handle which must be closed with xrtRunClose()
 *
 * The handle can be used repeatedly to start an execution of the
 * associated kernel.  This API allows application to manage run
 * handles without maintaining corresponding kernel handle.
 */
XCL_DRIVER_DLLESPEC
xrtRunHandle
xrtRunOpen(xrtKernelHandle kernelHandle);

/**
 * xrtRunSetArg() - Set a specific kernel argument for this run
 *
 * @rhdl:       Handle to the run object to modify
 * @index:      Index of kernel argument to set
 * @...:        The argument value to set.
 * Return:      0 on success, -1 on error
 *
 * Use this API to explicitly set specific kernel arguments prior
 * to starting kernel execution.  After setting all arguments, the
 * kernel execution can be start with xrtRunStart()
 */
XCL_DRIVER_DLLESPEC
int
xrtRunSetArg(xrtRunHandle rhdl, int index, ...);

/**
 * xrtRunUpdateArg() - Asynchronous update of kernel argument
 *
 * @rhdl:       Handle to the run object to modify
 * @index:      Index of kernel argument to update
 * @...:        The argument value to update.
 * Return:      0 on success, -1 on error
 *
 * Use this API to asynchronously update a specific kernel
 * argument of an existing run.  
 *
 * This API is only supported on Edge.
 */  
XCL_DRIVER_DLLESPEC
int
xrtRunUpdateArg(xrtRunHandle rhdl, int index, ...);

/**
 * xrtRunStart() - Start existing run handle
 *
 * @rhdl:       Handle to the run object to start
 * Return:      0 on success, -1 on error
 *
 * Use this API when re-using a run handle for more than one execution
 * of the kernel associated with the run handle.
 */
XCL_DRIVER_DLLESPEC
int
xrtRunStart(xrtRunHandle rhdl);

/**
 * xrtRunWait() - Wait for a run to complete
 *
 * @rhdl:       Handle to the run object to start
 * Return:      Run command state for completed run,
 *              or ERT_CMD_STATE_ABORT on error
 *
 * Blocks current thread until job has completed
 */
XCL_DRIVER_DLLESPEC
enum ert_cmd_state
xrtRunWait(xrtRunHandle rhdl);

/**
 * xrtRunWait() - Wait for a run to complete
 *
 * @rhdl:       Handle to the run object to start
 * @timeout_ms: Timeout in millisecond
 * Return:      Run command state for completed run, or
 *              current status if timeout.
 *
 * Blocks current thread until job has completed
 */
XCL_DRIVER_DLLESPEC
enum ert_cmd_state
xrtRunWaitFor(xrtRunHandle rhdl, unsigned int timeout_ms);

/**
 * xrtRunState() - Check the current state of a run
 *
 * @rhdl:       Handle to check
 * Return:      The underlying command execution state per ert.h
 */
XCL_DRIVER_DLLESPEC
enum ert_cmd_state
xrtRunState(xrtRunHandle rhdl);

/**
 * xrtRunSetCallback() - Set a callback function
 *
 * @rhdl:        Handle to set callback on
 * @state:       State to invoke callback on
 * @callback:    Callback function 
 * @data:        User data to pass to callback function
 *
 * Register a run callback function that is invoked when the
 * run changes underlying execution state to specified state.
 * Support states are: ERT_CMD_STATE_COMPLETED (to be extended)
 */
XCL_DRIVER_DLLESPEC
int
xrtRunSetCallback(xrtRunHandle rhdl, enum ert_cmd_state state,
                  void (* callback)(xrtRunHandle, enum ert_cmd_state, void*),
                  void* data);

/**
 * xrtRunClose() - Close a run handle
 *
 * @rhdl:  Handle to close
 * Return:      0 on success, -1 on error
 */
XCL_DRIVER_DLLESPEC
int
xrtRunClose(xrtRunHandle rhdl);

/// @endcond

#ifdef __cplusplus
}
#endif

#endif