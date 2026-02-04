# smb – Simple Message Broker (UDP)

Implements:
- smbbroker   (UDP broker, forwards messages, does not store)
- smbpublish  (sends one publish message and exits)
- smbsubscribe (subscribes to a topic and prints forwarded messages)

## Protocol (text UDP)
Subscriber -> Broker:
  SUB <topic> <port>
  UNSUB <topic> <port>

Publisher -> Broker:
  PUB <topic> <message>

Broker -> Subscriber:
  MSG <topic> <message>

Wildcard:
- Subscriber may subscribe to topic "#" to receive all topics.
- Publisher may NOT publish with "#".

## Build
make

## Run
Terminal 1:
  ./smbbroker
  (or ./smbbroker 8080)

Terminal 2:
  ./smbsubscribe localhost datum
  or:
  ./smbsubscribe localhost "#"
  (Press Ctrl+C to unsubscribe)

Terminal 3:
  ./smbpublish localhost datum "08.02.2021"
  ./smbpublish localhost test "hallo welt!"
