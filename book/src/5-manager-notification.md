# Notyfikowanie menedżera

Dotychczas rozważaliśmy dwa typy `service'u`: `simple` i `oneshot`. Charakteryzują się one tym, że `systemd` uznaje je za aktywne gdy, odpowiednio powstanie `fork` dla nowego procesu lub proces zakończy się z kodem 0. Moment aktywacji jest istotny, ponieważ to od niego zależy kiedy `systemd` może kontynuować uruchamianie kolejnych procesów. Czasami inicjalizacja procesu zabiera dużo czasu i chcielibyśmy sami określić kiedy proces jest gotowy. Weźmy pod uwagę poniższy skrypt.

```bash
#!/bin/bash

systemd-notify --status="Initiating…"

# Imitate time-absorbing initialization.
sleep 10

systemd-notify --ready

count=0
while true; do
    ((count++))
    systemd-notify --status="Working hard [${count}]"

    # Imitate time-absorbing processing.
    sleep 1
done
```

Program w pierwszej kolejności notyfikuje `systemd`, że jego status to _Initiating…_. Po dziesięciu sekundach notyfikuje swoją gotowość do pracy, a następnie co sekundę aktualizuje swój status. `systemd` udostępnia [API w języku C](https://www.freedesktop.org/software/systemd/man/sd_notify.html#) (`sd_notify`), które służy do wysyłania notyfikacji. Istnieje także [wrapper dla basha](https://www.freedesktop.org/software/systemd/man/systemd-notify.html) (`systemd-notify`), z którego tutaj korzystamy. Zapiszmy skrypt jako `my-notifier.sh`, nadajmy mu właściwe uprawnienia i skopiujmy do standardowego folderu.

```console
$ chmod +x my-notifier.sh
$ sudo cp my-notifier.sh /usr/local/bin/
```

Należy zwrócić uwagę, że notyfikacje występujące w skrypcie nie zadziałają jeżeli `systemd` nie wystosuje w tym celu odpowiedniego socketu do komunikacji z tym procesem. Aby tak się stało, `service` musi mieć typ `notify`.

```ini
[Unit]
Description=My Notifier

[Service]
Type=notify
ExecStart=/usr/local/bin/my-notifier.sh
```

Typ `notify` wymaga od usługi notyfikacji o właściwym uruchomieniu, a także umożliwia notyfikowanie innych istotnych zdarzeń. Zapiszmy nasz nowy `service` jako `my-notifier.service` i skopiujmy do odpowiedniego folderu.

```console
$ sudo cp my-notifier.service /etc/systemd/system/
```


Teraz włączmy nasz `service` i zobaczmy jego status. (Ponieważ właściwa aktywacja `service'u` następuje po 10 sekundach od jego wystartowania, polecenie `systemctl start` zablokuje konsolę, dlatego uruchomimy ten proces w tle.)

```console
$ sudo systemctl start my-notifier.service&
[1] 2950
$ systemctl status my-notifier.service
● my-notifier.service - My Notifier
   Loaded: loaded (/etc/systemd/system/my-notifier.service; static; vendor preset: enabled)
   Active: activating (start) since Tue 2019-10-01 14:48:08 CEST; 1s ago
 Main PID: 2956 (my-notifier.s)
   Status: "Initiating…"
    Tasks: 2 (limit: 4915)
   CGroup: /system.slice/my-notifier.service
           ├─2956 /bin/bash /usr/local/bin/my-notifier.sh
           └─2967 sleep 10

paź 01 14:48:08 KOM-135-LINUX systemd[1]: Starting My Notifier...
```

W powyższym zrzucie możemy doszukać się wpisu `Status: "Initiating…"`, czyli pierwsza notyfikacja dotarła do `systemd`. Ponadto widzimy też zapis `Active: activating`, który dodatkowo wskazuje, że usługa jeszcze nie jest w pełni uruchomiona. Po 10 sekundach ponownie sprawdzamy status.

```console
$ systemctl status my-notifier.service
● my-notifier.service - My Notifier
   Loaded: loaded (/etc/systemd/system/my-notifier.service; static; vendor preset: enabled)
   Active: active (running) since Tue 2019-10-01 14:48:18 CEST; 1s ago
 Main PID: 2956 (my-notifier.s)
   Status: "Working hard [1]"
    Tasks: 2 (limit: 4915)
   CGroup: /system.slice/my-notifier.service
           ├─2956 /bin/bash /usr/local/bin/my-notifier.sh
           └─3053 sleep 1

paź 01 14:48:08 KOM-135-LINUX systemd[1]: Starting My Notifier...
paź 01 14:48:18 KOM-135-LINUX systemd[1]: Started My Notifier.
[1]+  Done                    sudo systemctl start my-notifier.service
```

Tym razem usługa jest już w pełni aktywna, a jej status (`Status: "Working hard [1]"`) zmienia się co sekundę.

Domyślnie `systemd` daje usługom półtora minuty na wystartowanie (można zmienić ten okres poprzez odpowiednią wartość parametru `DefaultTimeoutStartSec` w konfiguracji `/etc/systemd/system.conf`). Jeżeli w tym czasie `service` typu `notify` nie wystosuje notyfikacji `READY=1`, usługa zostanie zabita (lub zrestartowana, w zależności od konfiguracji). Czasami nie wiemy jak długo potrwa inicjalizacja, dlatego istnieje specjalna notyfikacja `EXTEND_TIMEOUT_USEC=<czas>`, która informuje `systemd`, że potrzebne jest więcej czasu. Inna istotna notyfikacja to `WATCHDOG=1`, która informuje watchdoga, że usługa żyje. O tym dowiemy się w późniejszym rozdziale. Teraz jednak przejdźmy do jednej z najważniejszych cech `systemd` - aktywacji `socketami`.

