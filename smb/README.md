# smb – Simple Message Broker (UDP)

Implements:
- smbbroker   (UDP broker, forwards messages, does not store messages)
- smbpublish  (sends one publish message and exits)
- smbsubscribe (subscribes to a topic and prints forwarded messages)

Additional Implementations:
- smbsubscribe (unsubscribes from a topic when SIGINT is received)
- smbpublish_loop (sends messages periodically, e.g. every 30 seconds)
- smbpublish_interactive (interactive publisher with changeable topic)

## Protocol (text UDP)
Subscriber -> Broker:
  SUB <topic> <port>
  UNSUB <topic> <port>

Publisher -> Broker:
  PUB <topic> <message>

Broker -> Subscriber:
  MSG <topic> <message>

## Security (required)
All UDP packets are authenticated and encrypted with a shared secret.

### Key setup
`make` generates a random key file at `build/.key` (hex, 32 bytes) if it does not exist.
You can use it by passing `--key` (default path) or `--key=<path>`.

Fallbacks (if `--key` is not provided):
- `SMB_KEY` environment variable (hashed with SHA-256), or
- `build/.key` (if present).

Packets are authenticated with HMAC-SHA256 and encrypted with a SHA-256-based stream cipher.

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
- `make run-broker BROKER_ARGS="8080 --key"`
- `make run-publish PUBLISH_ARGS='localhost zimmer/temperatur "08.02.2021" --key'`
- `make run-publish-loop PUBLISH_LOOP_ARGS="localhost zimmer/temperatur 30 --key"`
- `make run-publish-interactive INTERACTIVE_ARGS="localhost zimmer/temperatur --key"`
- `make run-subscribe SUBSCRIBE_ARGS="localhost zimmer/# --key"`

If a message contains spaces, use:
- `make run-publish PUBLISH_ARGS='localhost zimmer/temperatur "hello world"'`

`smbpublish_loop` sends a counter + timestamp every N seconds:
- `./bin/smbpublish_loop localhost zimmer/temperatur 30`

`smbpublish_interactive` lets you change topic on the fly:
- `./bin/smbpublish_interactive localhost zimmer/temperatur`
- Commands: `/topic <newtopic>`, `/help`, `/quit`

## Run
Terminal 1:
  make run-broker BROKER_ARGS="8080 --key"
  or:
  ./bin/smbbroker 8080 --key

Terminal 2:
  make run-subscribe SUBSCRIBE_ARGS="localhost zimmer/# --key"
  or:
  ./bin/smbsubscribe localhost zimmer/# --key
  (Press Ctrl+C to unsubscribe)

Terminal 3:
  make run-publish PUBLISH_ARGS='localhost zimmer/temperatur "08.02.2021" --key'
  or:
  ./bin/smbpublish localhost zimmer/luftfeuchte "hallo welt!" --key
