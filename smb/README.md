# smb – Simple Message Broker (UDP)

Implements:
- smbbroker   (UDP broker, forwards messages, does not store)
- smbpublish  (sends one publish message and exits)
- smbpublish_loop (sends messages periodically, e.g. every 30 seconds)
- smbpublish_interactive (interactive publisher with changeable topic)
- smbsubscribe (subscribes to a topic and prints forwarded messages)

## Protocol (text UDP)
Subscriber -> Broker:
  SUB <topic> <port>
  UNSUB <topic> <port>

Publisher -> Broker:
  PUB <topic> <message>

Broker -> Subscriber:
  MSG <topic> <message>

Topics are hierarchical and use the form `ober/thema`, for example:
- `zimmer/temperatur`
- `zimmer/luftfeuchte`

Wildcards:
- Subscriber may subscribe to `ober/#` to receive all subtopics under `ober`.
- Subscriber may subscribe to `#` to receive all topics.
- Publisher may NOT publish with `#`.

## Build
make

Binaries are created in `bin/`:
- `bin/smbbroker`
- `bin/smbpublish <BROKER> <TOPIC> <MESSAGE>`
- `bin/smbpublish_loop <BROKER> <TOPIC> <INTERVAL_SECONDS> [PREFIX]`
- `bin/smbpublish_interactive <BROKER> <TOPIC>`
- `bin/smbsubscribe <BROKER> <TOPIC>`

Optional shortcuts:
- `make run-broker 8080`
- `make run-publish localhost zimmer/temperatur "08.02.2021"`
- `make run-publish-loop localhost zimmer/temperatur 30`
- `make run-publish-interactive localhost zimmer/temperatur`
- `make run-subscribe localhost zimmer/#`

If a message contains spaces, use:
- `make run-publish PUBLISH_ARGS='localhost zimmer/temperatur "hello world"'`

`smbpublish_loop` sends a counter + timestamp every N seconds:
- `./bin/smbpublish_loop localhost zimmer/temperatur 30`

`smbpublish_interactive` lets you change topic on the fly:
- `./bin/smbpublish_interactive localhost zimmer/temperatur`
- Commands: `/topic <newtopic>`, `/help`, `/quit`

## Run
Terminal 1:
  make run-broker 8080
  or:
  ./bin/smbbroker 8080

Terminal 2:
  make run-subscribe localhost zimmer/#
  or:
  ./bin/smbsubscribe localhost zimmer/#
  (Press Ctrl+C to unsubscribe)

Terminal 3:
  make run-publish localhost zimmer/temperatur "08.02.2021"
  or:
  ./bin/smbpublish localhost zimmer/luftfeuchte "hallo welt!"
