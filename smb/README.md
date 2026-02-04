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

Binaries are created in `bin/`:
- `bin/smbbroker`
- `bin/smbpublish <BROKER> <TOPIC> <MESSAGE>`
- `bin/smbsubscribe <BROKER> <TOPIC>`

Optional shortcuts:
- `make run-broker 8080`
- `make run-publish localhost datum "08.02.2021"`
- `make run-subscribe localhost datum`

If a message contains spaces, use:
- `make run-publish PUBLISH_ARGS='localhost datum "hello world"'`

## Run
Terminal 1:
  make run-broker 8080
  or:
  ./bin/smbbroker 8080

Terminal 2:
  make run-subscribe localhost datum
  or:
  ./bin/smbsubscribe localhost "#"
  (Press Ctrl+C to unsubscribe)

Terminal 3:
  make run-publish localhost datum "08.02.2021"
  or:
  ./bin/smbpublish localhost test "hallo welt!"
