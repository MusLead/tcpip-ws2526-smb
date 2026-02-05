SMB - SIMPLE MESSAGE BROKER (UDP)

GOAL
Develop a simple UDP-based message broker system with a broker (smbbroker),
a publisher (smbpublish), and a subscriber (smbsubscribe). Each message is a
pair of topic and message (both strings). The broker does not store messages;
it forwards messages only to subscribers that subscribed to the matching topic.

MAIN FEATURES
- smbbroker (UDP broker, forwards messages, does not store messages)
- smbpublish (sends one publish message and exits)
- smbsubscribe (subscribes to a topic and prints forwarded messages)
- wildcard # for subscribers to receive all topics

ADDITIONAL IMPLEMENTATIONS
- smbsubscribe (unsubscribes from a topic when SIGINT is received)
- smbpublish_loop (sends messages periodically, e.g. every 30 seconds)
- smbpublish_interactive (interactive publisher with changeable topic)
- hierarchical topics with prefix wildcard subscriptions (e.g. zimmer/#)

HOW IT WORKS
1. The broker (smbbroker) listens on UDP port 8080 and keeps an in-memory list
   of subscriptions; this list grows dynamically as new subscribers are added.
2. A subscriber binds to a local UDP port, sends "SUB <topic> <port>" to the
   broker, and waits for forwarded messages.
3. A publisher sends "PUB <topic> <message>" to the broker.
4. The broker matches the publish topic against subscriptions and forwards
   matching messages as "MSG <topic> <message>".
5. When a subscriber exits (SIGINT), it sends "UNSUB <topic> <port>" to remove
   the subscription.

PROTOCOL (TEXT OVER UDP)
Subscriber -> Broker:
- SUB <topic> <port>
- UNSUB <topic> <port>

Publisher -> Broker:
- PUB <topic> <message>

Broker -> Subscriber:
- MSG <topic> <message>

TOPIC RULES
Topics can be hierarchical (ober/thema) or single level (thema), 
for example:
- zimmer
- zimmer/temperatur
- zimmer/luftfeuchte

Wildcards:
- Subscriber may subscribe to ober/# to receive all subtopics under ober.
- Subscriber may subscribe to # to receive all topics.
- Publisher may NOT publish with #.

BUILD
Keep all source files in one folder (e.g., smb). Compile with gcc and place
outputs in bin/.

Commands:
  cd smb
  mkdir -p bin
  gcc -std=c11 -Wall -Wextra -O2 -D_POSIX_C_SOURCE=200112L smbbroker.c -o bin/smbbroker
  gcc -std=c11 -Wall -Wextra -O2 -D_POSIX_C_SOURCE=200112L smbpublish.c -o bin/smbpublish
  gcc -std=c11 -Wall -Wextra -O2 -D_POSIX_C_SOURCE=200112L smbsubscribe.c -o bin/smbsubscribe
  gcc -std=c11 -Wall -Wextra -O2 -D_POSIX_C_SOURCE=200112L smbpublish_loop.c -o bin/smbpublish_loop
  gcc -std=c11 -Wall -Wextra -O2 -D_POSIX_C_SOURCE=200112L smbpublish_interactive.c -o bin/smbpublish_interactive

PROJECT STRUCTURE
Important source files (all in the same folder, e.g., smb):
- smbbroker.c (UDP broker: manages subscriptions and forwards messages)
- smbpublish.c (publisher: sends one message and exits)
- smbsubscribe.c (subscriber: receives messages and prints them)
- smbpublish_loop.c (additional publisher: sends periodically)
- smbpublish_interactive.c (additional publisher: interactive CLI)
- smb.h (shared constants and helper functions)
- README.txt (documentation)

Executables required to run the Main Feature:
- bin/smbbroker
- bin/smbpublish
- bin/smbsubscribe

HOW TO EXECUTE (BINARIES)
Binaries are created in bin/:
- bin/smbbroker
- bin/smbpublish <BROKER> <TOPIC> <MESSAGE>
- bin/smbpublish_loop <BROKER> <TOPIC> <INTERVAL_SECONDS> [PREFIX]
  (sends counter + timestamp; optional prefix)
- bin/smbpublish_interactive <BROKER> <TOPIC>
- bin/smbsubscribe <BROKER> <TOPIC>

RUN (LOCAL EXAMPLE)
Terminal 1:
  cd smb
  ./bin/smbbroker 8080

Terminal 2:
  ./bin/smbsubscribe localhost zimmer/#

Terminal 3:
  ./bin/smbpublish localhost zimmer/temperatur "22.5C"

INTERACTIVE PUBLISHER
smbpublish_interactive lets you change topic on the fly:
  ./bin/smbpublish_interactive localhost zimmer/temperatur
Commands: /topic <newtopic>, /help, /quit

NOTES / LIMITATIONS
- UDP provides no delivery guarantee or ordering.
- The broker does not store messages.
- Clients always send to BROKER_PORT (8080). If the broker runs on a different
  port, update smb/smb.h and rebuild.

ASSIGNMENT NOTES (FROM AUFGABENSTELLUNG)
- The three core programs should be CLI-based and non-interactive at runtime.
- The broker should log incoming publish/subscribe requests to the console.
- Implementing periodic publishing and topic hierarchy with wildcards improves
  the grade.
