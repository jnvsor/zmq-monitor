zmq-monitor
===========

Gets key states from `/dev/input/*` files and uses them to keybind zmq messages. I use it for push-to-talk (And potentially other "On the fly" operations) in twitch streams from ffmpeg.

###usage

    zmq-montior [input-device] [shortcut...]

* Where `input-device` is something like: `/dev/input/by-id/keyboard`
* And shortcut is of the template: `keycombo "keydown signal" "keyup signal" delay`

`keycombo` is a + comma or space seperated list of linux key ids (As found in [`linux/input.h`](https://github.com/mirrors/linux/blob/master/include/uapi/linux/input.h#L200))

`delay` will delay sending the keyup signal for `delay * 0.01` seconds.

Example: `zmq-monitor /dev/input/by-id/usb-04d9_daskeyboard-event-kbd 29+56 "Parsed_volume_8 volume 1dB" "Parsed_volume_8 volume 0" 30 29+42 "Parsed_overlay_4 x 200" "" 0`

This loads the keyboard event, sets the combo to left control + left alt, sends "Parsed_volume_8 volume 1dB" on keydown, sends "Parsed_volume_8 volume 0" 0.3 seconds after keyup. It also checks for left control + left shift and sends "Parsed_overlay_4 x 200", but doesn't send anything on keyup.
