zmq-monitor
===========

Gets key states from `/dev/input/*` files and uses them to keybind zmq messages. I use it for push-to-talk (And potentially other "On the fly" operations) in twitch streams from ffmpeg.

###usage

    zmq-montior "/dev/input/by-id/keyboard" "keydown signal" "keyup signal"
