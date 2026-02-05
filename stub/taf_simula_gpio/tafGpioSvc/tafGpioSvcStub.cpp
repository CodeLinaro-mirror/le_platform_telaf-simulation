/* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @page c_tafgpio GPIO Service
 *
 * @rst :ref:`API reference <File taf_gpio_interface.h>` @endrst
 *
 * @rst :ref:`HAL APIs <File tafHalGpio.h>` @endrst
 *
 * <HR>
 *
 * This API is used by apps to control general-purpose digital input/output pins.
 *
 * A GPIO pin typically has one of the following features:
 * - Configured as an input pin or an output pin.
 * - If configured as an output, can be activated or deactivated.
 * - If configured as an input, can trigger an @e interrupt (asynchronous notification of state change).
 *
 * Pins also have a @e polarity mode:
 * - <b> active-high </b> polarity pin is read/written as a digital 1 (true) when its voltage is
 *  "high" and 0 (false) when its voltage is "low" (grounded).
 * - <b> active-low </b> pin is read/written as a digital 1 (true) when its voltage is
 *  "low" (grounded) and 0 (false) when its voltage is "high".
 *
 *
 * @section taf_gpio_binding IPC interfaces binding
 *
 * The functions of this API are provided by the @b tafGpioSvc application service.
 *
 * The following example illustrates how to bind to GPIO services.
 * @verbatim
   bindings:
   {
       clientExe.clientComponent.taf_gpio -> tafGpioSvc.taf_gpio
   }
   @endverbatim
 *
 * The following functions are used to configure the GPIO pin and lock the pin from being used by other
 * clients.
 * - taf_gpio_SetInput() - Configures the pin as an input pin.
 * - taf_gpio_SetEdgeSense() - Sets the edge sensing on an input pin (only works if you have an
 *   EventHandler).
 *
 * @verbatim
   res = taf_gpio_SetInput(inPinNum, TAF_GPIO_ACTIVE_HIGH, false);
    if(res == LE_OK) {
        LE_INFO("Gpio pin %d SetInput Successful", inPinNum);
    } else if (res == LE_BUSY) {
        LE_INFO("Gpio pin %d SetInput results in GPIO_BUSY", inPinNum);
    } else if (res == LE_OUT_OF_RANGE) {
        LE_INFO("Gpio pin %d is out of range", outPinNum);
    }  else
        LE_INFO("Gpio pin %d SetInput results in IO ERROR", inPinNum);
   @endverbatim
 *
 * To set the level of an output pin and lock the pin from being used by other clients, call
 * taf_gpio_Activate() or taf_gpio_Deactivate().
 *
 * To poll the value of an input pin, call taf_gpio_Read().
 *
 * Use the ChangeEvent to register a notification callback function to be called when the
 * state of an input pin changes. The type of edge detection can then be modified by calling
 * taf_gpio_SetEdgeSense() or taf_gpio_DisableEdgeSense().
 * @b NOTE: The client will be killed in the following scenarios.
 * - If the GPIO object reference is NULL or not initialized.
 * - When unable to set edge detection correctly.
 *
 * @verbatim
   gpiohandlerRef = taf_gpio_AddChangeEventHandler(inPinNum, TAF_GPIO_EDGE_BOTH, false,
            GpioChangeCallback, NULL);
   @endverbatim
 *
 * The following functions can be used to read the current setting for a GPIO pin even if the GPIO
 * pin is locked by another client. In a Linux environment these values are
 * read from the sysfs and reflect the actual value at the time the function is called.
 * - taf_gpio_IsOutput() - Is the pin currently an output?
 * - taf_gpio_IsInput() - Is the pin currently an input?
 * - taf_gpio_IsActive() - Is an output pin currently being driven? (corresponds to the value file
 * in sysfs)
 * - taf_gpio_GetPolarity() - Retrieves the current polarity (active-low or active-high).
 * - taf_gpio_GetEdgeSense() - What edge sensing has been enabled on an input pin?
 *
 * <HR>
 *
 */


#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
/**
 * Configures the pin as an input pin.
 *
 * @return
 *     - LE_OK           Succeeded.
 *     - LE_BUSY         GPIO is busy or locked by another client.
 *     - LE_IO_ERROR     Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_gpio_SetInput
(
    uint8_t pinNum,
        ///< [IN] GPIO pin number.
    taf_gpio_Polarity_t polarity,
        ///< [IN] Active-high or active-low.
    bool lock
        ///< [IN] Set to True to lock this GPIO pin from being used by other clients.
)
{
    LE_INFO("[simulation]: set input (%d): %d + %d", pinNum, polarity, lock);
    return LE_OK;
}
//--------------------------------------------------------------------------------------------------
/**
 * Sets an output pin to Active state.
 * @b WARNING: Valid only for output pins.
 *
 * @return
 *     - LE_OK           Succeeded.
 *     - LE_BUSY         GPIO is busy or locked by another client.
 *     - LE_IO_ERROR     Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_gpio_Activate
(
    uint8_t pinNum,
        ///< [IN] GPIO pin number.
    bool lock
        ///< [IN] Set to True to lock this GPIO pin from being used by other clients.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Sets an output pin to Inactive state.
 * @b WARNING: Valid only for output pins.
 *
 * @return
 *     - LE_OK           Succeeded.
 *     - LE_BUSY         GPIO is busy or locked by another client.
 *     - LE_IO_ERROR     Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_gpio_Deactivate
(
    uint8_t pinNum,
        ///< [IN] GPIO pin number.
    bool lock
        ///< [IN] Set to True to lock this GPIO pin from being used by other clients.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Reads the value of an input GPIO pin.
 *
 * @return
 *     - TAF_GPIO_BUSY  GPIO pin is locked for the pin.
 *     - TAF_GPIO_OFF   GPIO pin voltage is low.
 *     - TAF_GPIO_ON    GPIO pin voltage is high.
 *
 * @b NOTE: Invalid to read an output pin.
 */
//--------------------------------------------------------------------------------------------------
taf_gpio_State_t taf_gpio_Read
(
    uint8_t pinNum,
        ///< [IN] GPIO pin number.
    bool lock
        ///< [IN] Set to True to lock this GPIO pin from being used by other clients.
)
{
    return (taf_gpio_State_t) 0;
}
//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_gpio_ChangeEvent'
 *
 * Registers a callback function to be called when an input pin changes state.
 *
 * If registering fails, because the handler cannot be registered, setting the
 * edge detection fails, or the GPIO pin is locked by other client then this call
 * returns a NULL reference.
 */
//--------------------------------------------------------------------------------------------------
taf_gpio_ChangeEventHandlerRef_t taf_gpio_AddChangeEventHandler
(
    uint8_t pinNum,
        ///< [IN] GPIO pin number.
    taf_gpio_Edge_t trigger,
        ///< [IN] Change(s) that should trigger the callback to be called.
    bool lock,
        ///< [IN] Set to True to lock this GPIO pin from being used by other clients.
    taf_gpio_ChangeCallbackFunc_t handlerPtr,
        ///< [IN] The callback function.
    void* contextPtr
        ///< [IN]
)
{
    LE_INFO("[simulation]: change event handler registered. (nothing to do)");
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_gpio_ChangeEvent'
 */
//--------------------------------------------------------------------------------------------------
void taf_gpio_RemoveChangeEventHandler
(
    taf_gpio_ChangeEventHandlerRef_t handlerRef
        ///< [IN]
)
{
    LE_INFO("[simulation]: change event handler droped. (nothing to do)");
}
//--------------------------------------------------------------------------------------------------
/**
 * Sets the edge detection mode. This function can only be used when a handler is registered
 * in order to prevent interrupts being generated and not handled.
 *
 * @return
 *     - LE_OK           Succeeded.
 *     - LE_BUSY         GPIO is busy or locked by another client.
 *     - LE_IO_ERROR     Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_gpio_SetEdgeSense
(
    uint8_t pinNum,
        ///< [IN] GPIO pin number.
    taf_gpio_Edge_t trigger,
        ///< [IN] Change(s) that should trigger the callback to be called.
    bool lock
        ///< [IN] Set to True to lock this GPIO pin from being used by other clients.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Turns off edge detection of the input GPIO pin.
 *
 * @return
 *     - LE_OK           Succeeded.
 *     - LE_BUSY         GPIO is busy or locked by another client.
 *     - LE_IO_ERROR     Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_gpio_DisableEdgeSense
(
    uint8_t pinNum,
        ///< [IN] GPIO pin number.
    bool lock
        ///< [IN] Set to True to lock this GPIO pin from being used by other clients.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Checks if the pin is configured as an output.
 *
 * @return
 * True if output; false if input.
 */
//--------------------------------------------------------------------------------------------------
bool taf_gpio_IsOutput
(
    uint8_t pinNum
        ///< [IN] GPIO pin number.
)
{
    return false;
}
//--------------------------------------------------------------------------------------------------
/**
 * Checks if the pin is configured as an input.
 *
 * @return
 * True if output; false if input.
 */
//--------------------------------------------------------------------------------------------------
bool taf_gpio_IsInput
(
    uint8_t pinNum
        ///< [IN] GPIO pin number.
)
{
    return false;
}
//--------------------------------------------------------------------------------------------------
/**
 * Returns the I/O name.
 *
 * @return
 * Name in string format
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_gpio_GetName
(
    uint8_t pinNum,
        ///< [IN] GPIO pin number.
    char* name,
        ///< [OUT] I/O name as output
    size_t nameSize
        ///< [IN]
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the current value of edge sensing.
 *
 * @return
 * The current configured edge value.
 *
 * @b NOTE: Invalid to read the edge sense of an output pin.
 */
//--------------------------------------------------------------------------------------------------
taf_gpio_Edge_t taf_gpio_GetEdgeSense
(
    uint8_t pinNum
        ///< [IN] GPIO pin number.
)
{
    return (taf_gpio_Edge_t) 0;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the current value of the pin polarity.
 *
 * @return
 * The current pin polarity.
 */
//--------------------------------------------------------------------------------------------------
taf_gpio_Polarity_t taf_gpio_GetPolarity
(
    uint8_t pinNum
        ///< [IN] GPIO pin number.
)
{
    return (taf_gpio_Polarity_t) 0;
}
//--------------------------------------------------------------------------------------------------
/**
 * Checks if the pin is currently active.
 *
 * @return
 * True if active; false if inactive.
 *
 * @b NOTE: This can only be used on output pins.
 */
//--------------------------------------------------------------------------------------------------
bool taf_gpio_IsActive
(
    uint8_t pinNum
        ///< [IN] GPIO pin number.
)
{
    return false;
}

COMPONENT_INIT
{
    LE_INFO("[simulation] gpio svc stub code init.");
}
