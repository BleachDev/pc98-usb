
# PC-98 To USB Converter

This converter allows PC-98 keyboards to be connected to modern computers via an Arduino

## How to use

1. Connect your keyboard to your Arduino according to the chart below, you can move the pins if you also change them in the Arduino code.
<details>
<summary>Connection Chart</summary>
<img src="https://github.com/BleachDev/pc98-usb/assets/36450388/82309194-2d2c-422e-bab7-67e6709e4af6">
</details>

2. Upload `pc98_usb.ino` to your Arduino
3. Short your Arduino then flash `Arduino-keyboard-0.3.hex` to it using dfu-programmer or Flip (You can undo it by flashing `Arduino-usbserial-uno.hex`)
4. Your Arduino/PC-98 Keyboard should act like a USB keyboard.

## Layout
![layout](https://github.com/BleachDev/pc98-usb/assets/36450388/37f0fb0f-8e3a-4e45-aff5-76e4446f818c)
Brown = Only found on PC-98XL  
Cyan = Only found on newer models  
Red = Key remappings when standardize mode is enabled

## Commands

Even though this adapter currently doesn't support sending commands here is a list of the commands the keyboard will accept.

##### READ
- `0xFA` Acknowledged
- `0xFB` Not Ready?
- `0xFC` Not Acknowledged
##### WRITE
- `0x95` Enables/Disables the Gui & Menu keys for the keyboards that have them  
`0x95 0x00` Disable Gui & Menu keys  
`0x95 0x03` Enable Gui & Menu keys  

- `0x96` Get current JIS conversion mode  
Returns `0xA0 0x86` = Automatic conversion mode  
Returns `0xA0 0x85` = Normal mode  

- `0x99` Unknown, returns `0xFB`  

- `0x9C` Keyboard repeat speed  
Format: `0x9C 01111111`  
Bit 7 - Unused  
Bit 6-5 - Delay before repeat (11 = 1000ms, 10 = 500ms, 01 = 500ms, 00 = 250ms)  
Bit 4-0 - Repeat speed (11111 = Slowest, 00001 = Fastest (default))  

- `0x9D` Keyboard LED Settings  
`0x9D 0x60` Returns the LED state in this format: `0000KC0N`, K = Kana, C = Caps, N = Num  
`0x9D 0111KC0N` Sets the LED state, K = Kana, C = Caps, N = Num

- `0x9F` Returns the keyboard type
Returns `0xA0 0x80` on new keyboards

## Key repeating

Repeating scancodes are sent internally from the PC-98 Keyboard, some newer models have a command to stop key repeating but this adapter handles it completely in software so it works on all models.  
As a result there is a ~20ms delay (can be changed) when typing but its barely noticeable and no keys are dropped/missed (see typing demo below)

## Typing demo

https://github.com/BleachDev/pc98-usb/assets/36450388/fe17ed0a-2b38-473e-a3f6-767af67debd1

## Notes

I have only been able to test this with a PC-9801R Keyboard but _most_ other models should work.  
The PC-9801V might not work due to it [requiring any command to init](https://github.com/tmk/tmk_keyboard/wiki/PC-9801-Keyboard#connection-order).  
The [PTOS Keyboard](https://www.youtube.com/watch?v=cvOQu0tBK8I) may or may not work since it uses quite a different layout.

If you have any PC-98 Keyboard please let me know if it works or not.

## Resources

[TMK PC-9801 Wiki](https://github.com/tmk/tmk_keyboard/wiki/PC-9801-Keyboard)  
[Hardware Manual - Keyboard Protocol](https://archive.org/stream/PC9800TechnicalDataBookHARDWARE1993/PC-9800TechnicalDataBook_HARDWARE1993#page/n151)  
[Hardware Manual - Keyboard Interface](https://archive.org/stream/PC9800TechnicalDataBookHARDWARE1993/PC-9800TechnicalDataBook_HARDWARE1993#page/n355)  
[Keyboard Interface & Commands](http://www.webtech.co.jp/company/doc/undocumented_mem/io_kb.txt)  
[Different Keyboard Models](https://radioc.web.fc2.com/column/pc98bas/pc98kbdmouse_en.htm)  
[Keycodes & Miscellaneous](https://ixsvr.dyndns.org/usb2pc98)
