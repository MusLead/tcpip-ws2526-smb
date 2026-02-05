**smb – Simple Message Broker (UDP)**

**Goal**  
Develop a simple UDP-based message broker system with a broker (`smbbroker`), a publisher (`smbpublish`), and a subscriber (`smbsubscribe`). Each message is a tuple of `topic` and `message` (both strings). The broker does not store messages; it forwards messages only to subscribers that subscribed to the matching topic.

**Main Features**  
- `smbbroker` (UDP broker, forwards messages, does not store messages)
- `smbpublish` (sends one publish message and exits)
- `smbsubscribe` (subscribes to a topic and prints forwarded messages)
- wildcard `#` for subscribers to receive all topics

**Additional Implementations**
- `smbsubscribe` (unsubscribes from a topic when SIGINT is received)
- `smbpublish_loop` (sends messages periodically, e.g. every 30 seconds)
- `smbpublish_interactive` (interactive publisher with changeable topic)
- hierarchical topics with prefix wildcard subscriptions (e.g. `zimmer/#`)

**How It Works**  
1. The broker (`smbbroker`) listens on UDP port 8080 and keeps an in-memory list of subscriptions; this list grows dynamically as new subscribers are added.  
2. A subscriber binds to a local UDP port, sends `SUB <topic> <port>` to the broker, and waits for forwarded messages.  
3. A publisher sends `PUB <topic> <message>` to the broker.  
4. The broker matches the publish topic against subscriptions and forwards matching messages as `MSG <topic> <message>`.  
5. When a subscriber exits (SIGINT), it sends `UNSUB <topic> <port>` to remove the subscription.

**Protocol (text over UDP)**  
Subscriber → Broker  
- `SUB <topic> <port>`  
- `UNSUB <topic> <port>`

Publisher → Broker  
- `PUB <topic> <message>`

Broker → Subscriber  
- `MSG <topic> <message>`

**Topic Rules**  
Topics are usually hierarchical like `ober/thema`, but a single level is also allowed, for example:
- `zimmer`
- `zimmer/temperatur`
- `zimmer/luftfeuchte`

Wildcards:
- Subscriber may subscribe to `ober/#` to receive all subtopics under `ober`.
- Subscriber may subscribe to `#` to receive all topics.
- Publisher may NOT publish with `#`.

**Build**  
Keep all source files in one folder (e.g., `smb`). Compile with `gcc` and place outputs in `bin/`.
```sh
cd smb
mkdir -p bin
gcc -std=c11 -Wall -Wextra -O2 -D_POSIX_C_SOURCE=200112L smbbroker.c -o bin/smbbroker
gcc -std=c11 -Wall -Wextra -O2 -D_POSIX_C_SOURCE=200112L smbpublish.c -o bin/smbpublish
gcc -std=c11 -Wall -Wextra -O2 -D_POSIX_C_SOURCE=200112L smbsubscribe.c -o bin/smbsubscribe
gcc -std=c11 -Wall -Wextra -O2 -D_POSIX_C_SOURCE=200112L smbpublish_loop.c -o bin/smbpublish_loop
gcc -std=c11 -Wall -Wextra -O2 -D_POSIX_C_SOURCE=200112L smbpublish_interactive.c -o bin/smbpublish_interactive
```

**Project Structure**  
Important source files (all in the same folder, e.g., `smb`):
- `smbbroker.c` (UDP broker: manages subscriptions and forwards messages)
- `smbpublish.c` (publisher: sends one message and exits)
- `smbsubscribe.c` (subscriber: receives messages and prints them)
- `smbpublish_loop.c` (optional publisher: sends periodically)
- `smbpublish_interactive.c` (optional publisher: interactive CLI)
- `smb.h` (shared constants and helper functions)
- `Makefile` (optional build helper, not required)
- `README.md` / `README.txt` (documentation)

Executables required to run the core system:
- `bin/smbbroker`
- `bin/smbpublish`
- `bin/smbsubscribe`

**How to Execute (Binaries)**  
Binaries are created in `bin/`:
- `bin/smbbroker`
- `bin/smbpublish <BROKER> <TOPIC> <MESSAGE>`
- `bin/smbpublish_loop <BROKER> <TOPIC> <INTERVAL_SECONDS> [PREFIX]`  
  (sends counter + timestamp; optional prefix)
- `bin/smbpublish_interactive <BROKER> <TOPIC>`
- `bin/smbsubscribe <BROKER> <TOPIC>`

**Run (local example)**  
Terminal 1:
```sh
cd smb
./bin/smbbroker 8080
```

Terminal 2:
```sh
./bin/smbsubscribe localhost zimmer/#
```

Terminal 3:
```sh
./bin/smbpublish localhost zimmer/temperatur "22.5C"
```

**Interactive Publisher**  
`smbpublish_interactive` lets you change topic on the fly:
```sh
./bin/smbpublish_interactive localhost zimmer/temperatur
```
Commands: `/topic <newtopic>`, `/help`, `/quit`

**Notes / Limitations**  
- UDP provides no delivery guarantee or ordering.  
- The broker does not store messages.  
- Clients always send to `BROKER_PORT` (8080). If the broker runs on a different port, update `smb/smb.h` and rebuild.

**Assignment Notes (from Aufgabenstellung)**  
- The three core programs should be CLI-based and non-interactive at runtime.  
- The broker should log incoming publish/subscribe requests to the console.  
- Implementing periodic publishing and topic hierarchy with wildcards improves the grade.  
