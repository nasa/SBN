# SBN Demo

The talker_app generates and sends a counter that it increments each iteration. The listener_app receives and processes talker_app's messages sent over the software message bus. When a message is received, the received counter value is then transmitted back to the software bus with a message ID that talker_app is subscribed to. talker_app receives the response and checks if the received counter value matches its own. If they match, it increments its counter. If they don't match, it logs a mismatch. This setup effectively creates a loop where the talker sends data, the listener acknowledges it, and the talker uses the acknowledgement to generate new data, demonstrating a basic communication pattern over SBN.

## Execution

The script will compile the cFS installation with SBN and a
talker/listener example enabled, and launch it, one per CPU
enabled in the `targets.cmake` file.

Instance 1 will be the talker and instance 2 will be the listener