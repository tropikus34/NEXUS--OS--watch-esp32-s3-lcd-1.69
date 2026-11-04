## Дополнение 6.7.0

NXHELLO: VARIANT=1. NXGETCONFIG: variant 0…6. NXVARIANT <0…6>: OK VARIANT, ERR RANGE или ERR SAVE. Запись NVS до изменения RAM.

# NEXUS Studio protocol v1 / firmware 6.7.0

USB CDC, 115200 baud, ASCII lines ending in LF. One request at a time. Start with `NXHELLO`; require `STUDIO=1`; for OSD require `OSD=1`. `NXGETCONFIG` replies `CONFIG ` followed by schema-1 JSON. All numeric command arguments below are decimal unless explicitly noted. Commands require exact argument count and range; replies beginning `ERR` are failures. Settings ACK is emitted after persistence is attempted. Multi-command writes are not atomic. See NexusStudio.h for authoritative ranges/behavior.

| Request | Purpose |
|---|---|
| NXGETCONFIG | Settings and live diagnostic JSON |
| NXSYS brightness timeout format24 blink quiet private autoReconnect eco dnd | Screen/shared system flags, OK SETTINGS |
| NXOWNER NAME | 1–8 uppercase ASCII letters/digits/space/-/_, nonblank; OK SETTINGS |
| NXALARM slot h m on | Slots 0–4, OK SETTINGS |
| NXTIMERSET seconds | 1–5999 seconds; rejects running/ringing timer |
| NXTIMER START / PAUSE / RESET | Timer control and checkpoint, OK TIMER |
| NXSETTIME year month day h m s | 2000–2099, local date/time; OK TIME RTC=0 or 1 |
| NXHEALTHFULL height weight10 strideMm sensitivity goal moveMinutes reminders enabled | Body profile/activity, OK SETTINGS |
| NXAPPEAR theme accent secondary style wallpaper specialization offset | Full appearance selection, OK SETTINGS |
| NXCOLORS bg panel text muted accent secondary | Six RGB565 integer colors, OK COLORS |
| NXFACE seconds compA compB wallMix | Seconds bit, selectors 0–6, image mix 0–100; OK FACE |
| NXSOUND hz ms | 200–5000 Hz, 40–1000 ms; OK SOUND |
| NXSOUNDTEST | Obeys quiet/DND/alert state |
| NXPAIR | Open pairing window for two minutes |
| NXFORGET CONFIRM | Forget phone |
| NXCLEARMSG CONFIRM | Clear message list |
| NXWAKE | Wake display |
| NXSTOPALERT | Stop current alert |
| NXREBOOT CONFIRM | Persist settings/activity/timer then restart |
| NXWALLINFO | WALLINFO SIZE=decimal CRC=hex; zero size means absent |
| NXWALLREAD offset | Offset aligned to 256; WALL offset lowercaseHexBytes |
| NXCAPACITY mAh | Nominal profile only, 10–10000; OK CAPACITY |
| NXBATTERY / NXSTATUS / NXACTIVITY | Read diagnostics |
| NXNOTIFY | Inject test notification |
| NXDELETE | Delete stored background |
| NXFORMAT CONFIRM | Prepare missing/unready LittleFS, firmware rejects when already ready |

Appearance theme indices 0–6: ONYX, AURUM, GLACIER, EMBER, NEBULA, ATELIER, VECTOR; 7 = custom. Custom style 0–1. Specializations 0–6 in UI order. Travel offset -720..840 minutes, multiples of 30. Complications: 0 automatic, 1 steps, 2 timer, 3 alarm, 4 messages, 5 battery, 6 hidden. Timeout is the firmware preset index 0–4, not a number of seconds.

Wallpaper upload uses existing `NXBG1 length hexCRC` handshake and binary RGB565 little-endian, 240×280×2 = 134400 bytes. Follow READY/chunk acknowledgement/final OK behavior in NexusTheme.h and the standalone studio. Upload chunks are 512 bytes; final chunk 256. Download chunks are 256 bytes. CRC-32 is reflected polynomial 0xEDB88320, initial/final XOR 0xffffffff. Device stages upload before replacing the active file.

CONFIG includes live values and current runtime time/timer. Portable projects intentionally exclude runtime sessions, time restoration, BLE secrets, message/step history. Commands are local USB operations without separate application authentication; physical port access is the trust boundary. No charging-control command exists.

## OSD layout v1

`NXOSD <170 hexadecimal characters>` replies `OK OSD` only after the complete layout is saved to the single NVS key `osd`; no per-element writes are exposed. Max line buffer is now 256 bytes including terminating NUL. Oversized lines are discarded to LF.

The payload has 85 bytes: header `[version=1, enabled=0|1, count=13]`; thirteen six-byte records `[x:uint8, yLo:uint8, yHi:uint8, size:1|2, visible:0|1, color:0..3]`; four-byte little-endian CRC-32 of bytes 0..80. Y is little-endian. Item 11 (BOOT hint) only supports size 1. Colors: text, accent, muted, secondary. All items, including hidden ones, must fit the 240×280 screen. Zero visible items is allowed. Invalid version/count/hex/length/CRC/bounds and NVS write failure do not change the active layout.

CONFIG contains `osd:{version:1,enabled:0|1,items:[{x,y,size,on,color},...]}`. Element order: time, seconds, date, battery, owner, steps, timer, alarm, messages, Bluetooth, specialization, BOOT hint, AM/PM. Later items draw over earlier ones. The editor selection outline does not alter this order.

Dimensions (width × height): time 142×44 or 206×64; seconds and AM/PM 36×18 or 72×36; hint 216×18; other fields 108×18 or 216×36. Coordinates are upper-left pixels; color and background come from the current theme. Text is clipped by complete glyphs to its declared box. Seconds and AM/PM also obey global seconds and 12/24h settings. The full-screen custom clock replaces the built-in clock only when enabled; other screens and alert overlays retain their original layout.

Layout blob replacement is atomic at one Preferences/NVS key; saving the whole studio project (other settings plus wallpaper) remains a sequence of commands, not a single transaction. Old projects without `osd` import with the default disabled layout. Legacy `.nxtheme` imports disable OSD to show the native imported theme, retaining the local OSD coordinates.

## Display/storage diagnostics 6.7.0

NXHELLO adds `FSINFO=1 BUFFER=1` capability tokens. CONFIG.live adds fsPartitionBytes (data partition labelled spiffs, 0 if missing), fsTotal/fsUsed (mounted filesystem bytes), and buffered (1 when using the off-screen canvas). The existing fields remain compatible.

NXBG1 now rejects `ERR FS_NO_PARTITION`, `ERR NO_FILESYSTEM`, or `ERR FS_NO_SPACE` before starting a transfer; required free space is WALL_BYTES+8192 for metadata margin. NXFORMAT CONFIRM rejects a missing partition without trying to format. Preparing a filesystem never creates/repartitions flash.

The studio performs its own preflight before any writes in Apply All with a pending background and checks NXWALLINFO size/CRC after the final upload ACK. An unprepared filesystem, missing partition and low space preserve the local project/image and produce an actionable message.
