# ESP32 MOD Player

This project is an MOD music player based on ESP32, capable of playing MOD music files. MOD is an 8-bit sample tracker file format used on the Amiga computer.
![1000043204](https://github.com/jsjsjsjsjsjsjson/ESP32ModPlayer/assets/86410439/08d41d47-c43a-4c29-82e3-08360a252585)

## Features

- Supports playing MOD music files
- Can display the waveform of MOD music files
- Supports volume adjustment, frequency adjustment, and more

## Dependencies

- FreeRTOS
- pwm_audio
- ESP-IDF
- SSD1306

## Usage

1. Use the `byte_tool.py` in the `extratools` folder to convert MOD music files into C language arrays.
2. Include the converted MOD music files in `main.c` for playback. Several songs are already included.
3. Compile and flash the project onto the ESP32 development board.
4. Plug the speaker into GPIO12 (or you can change the pwm_audio_config structure to make the audio output into any other PWM pin)
5. Run the project to start playing MOD music.

## Possible Future Features

- More effects
- Reading MOD files from SD card or flash

## Notes

- This project only supports MOD music files in specific formats.
- If you do not have an SSD1306 screen, remove the code in the app_main function that starts the "wave_view" task. This will not affect any playback functionality
- Make sure to have the ESP-IDF development environment installed before compiling and flashing the project.

## Contribution

Contributions and suggestions are welcome. You can contribute by submitting issues or pull requests.

## Acknowledgments

Thanks to all contributors and the open-source community for their support of this project.

## References

- [FreeRTOS](https://www.freertos.org/)
- [ESP-IDF](https://github.com/espressif/esp-idf)
- [SSD1306](https://github.com/nopnop2002/esp-idf-ssd1306)

Feel free to contact the author for any questions or suggestions.
