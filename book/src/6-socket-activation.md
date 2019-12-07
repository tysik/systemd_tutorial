# Aktywacja socketami

Jak wspomniano w rozdziale pierwszym, `systemd` stawia na możliwie duże zrównoleglenie procesu uruchamiania systemu. Głównym narzędziem służącym temu szczytnemu celowi jest tzw. aktywacja `socketami`. `sockety`, oprócz standardowego rozumienia, stanowią szczególny przypadek `unitów`, które definiują kanał komunikacyjny pomiędzy procesami (ICP).

Przeanalizujmy następującą sytuację. `service` `X` świadczy pewną usługę dla innych procesów (jest serwerem). W tym celu tworzy on plik wymiany danych (np. socket unixowy lub `FIFO`), poprzez który inne procesy mogą się z nim komunikować (są klientami). Odruchowo nasuwa się zależność - klienci wymagają do działania `service'u` `X`, więc powinny być uruchomione po nim. Jednak technicznie rzecz biorąc, klienci wymagają jedynie socketu, do którego mogą wysyłać swoje zapytania. Jeżeli `X` jeszcze się nie uruchomił lub tymczasowo przestał działać, socket będzie kolejkował te zapytania do późniejszego przetworzenia. Osiągamy dzięki temu faktyczną niezależność modułów. Co więcej, niektóre `service'y` mogą pozostać nieaktywne dopóty, dopóki nie pojawi się pierwsze zapytanie. I na tym właśnie polega aktywacja `socketami` - wstrzymaj się z uruchomieniem `unitu` aż do chwili, kiedy faktycznie jest potrzebny.

Aktywację `socketami` przeanalizujemy w oparciu o prosty system logowania, który udostępnia plik `FIFO` dla klientów i zapisuje wiadomości w pliku tekstowym. Zacznijmy od skryptu.

```bash
#!/bin/bash

pipe=/tmp/my-logger.fifo
logbook=/tmp/my-logger.log

function save-logs() {
    logs=("$@")
    for log in "${logs[@]}"; do
        echo ${log} >> ${logbook}
    done
}

if [[ ! -p ${pipe} ]]; then
    mkfifo ${pipe}
fi

if [[ ! -f ${logbook} ]]; then
    touch ${logbook}
fi

while true; do
    logs=()
    for i in {1..5}; do
        if read line < ${pipe}; then
            if [[ ${line} == 'quit' ]]; then
                # Store remaining logs.
                save-logs "${logs[@]}"
                exit 0
            fi
            logs+=("[$(date +%Y-%m-%d\ %H:%M:%S)]  > ${line} <")
        fi
    done
    save-logs "${logs[@]}"
done

```

Jak powiedział pewien mój kolega: "Bash jest ciekawym językiem, dopóki nie próbuje się w nim programować." Skrypt nasłuchuje na nowe dane przychodzące z pliku `pipe`, okrasza je datą, a następnie kolejkuje i zrzuca do pliku `logbook` po pięć sztuk naraz. Wysłanie słowa `quit` kończy pracę loggera. Zapiszmy nasz skrypt jako `my-logger.sh`, nadajmy mu odpowiednie uprawnienia i przetestujmy czy działa.

```console
$ chmod +x my-logger.sh
$ ./my-logger.sh

(...a w osobnej konsoli...)

$ echo "1" > /tmp/my-logger.fifo
$ echo "2" > /tmp/my-logger.fifo
$ echo "3" > /tmp/my-logger.fifo
$ echo "quit" > /tmp/my-logger.fifo

$ cat /tmp/my-logger.log
[2019-10-03 09:31:29] > 1 <
[2019-10-03 09:31:35] > 2 <
[2019-10-03 09:31:37] > 3 <
```

Wygląda na to, że wszystko działa poprawnie. Skopiujmy zatem nasz skrypt w odpowiednie miejsce.

```console
$ sudo cp my-logger.sh /usr/local/bin/
```

Teraz przejdźmy do zdefiniowania `service'u`.

```ini
[Unit]
Description=My Logger

[Service]
ExecStart=/usr/local/bin/my-logger.sh
```

Nie ma tutaj nic nowego. Zapiszmy definicję jako `my-logger.service` we właściwym katalogu.

```console
$ sudo cp my-logger.service /etc/systemd/system/
```

A teraz do sedna. Nie możemy zainstalować naszego `service'u`, ponieważ nie dodaliśmy w nim sekcji `[Install]`. Możemy uruchomić go ręcznie, ale równie dobrze moglibyśmy po prostu uruchomić ręcznie nasz skrypt. Zamiast tego chcielibyśmy aktywować skrypt gdy pojawią się pierwsze dane. W tym celu dodamy definicję `socketu`, który będzie aktywował nasz `service` podług potrzeb.

```ini
[Unit]
Description=Socket for My Logger

[Socket]
ListenFIFO=/tmp/my-logger.fifo

[Install]
WantedBy=sockets.target
```

Dodaliśmy nową sekcję `[Sockets]`, w której konfigurujemy kanał komunikacyjny oraz sposoby aktywacji. Parametr `ListenFIFO` przyjmuje ścieżkę do pliku `FIFO`, który będzie tworzony podczas włączenia `socketu`. Lista dostępnych atrybutów tej sekcji znajduje się w [dokumentacji](https://www.freedesktop.org/software/systemd/man/systemd.socket.html).

No ale zaraz, zaraz. Przecież nigdzie nie wskazaliśmy _co_ ma zostać uruchomione, gdy w pliku wymiany danych pojawią się pierwsze dane! Nie musimy tego robić, ponieważ domyślnie `socket` uruchamia `service` o tej samej nazwie - i tutaj z tego skorzystamy. (Gdybyśmy jednak chcieli uruchomić `service` o innej nazwie, możemy skorzystać z parametru `Service`.) Zapiszmy definicję `socketu` jako `my-logger.socket`, skopiujmy ją do odpowiedniego folderu.

```console
$ sudo cp my-logger.socket /etc/systemd/system/
```

W sekcji `[Install]` wskazaliśmy, że nasz `socket` można zainstalować do `sockets.target` - czyli specjalnego `targetu`, który jest aktywowany we wstępnej fazie uruchamiania systemu. Nie jest to wymagane, ale jeśli chcemy uruchamiać nasz `socket` przy starcie systemu, warto podpiąć go właśnie pod ten `target`.

Czas na przetestowanie naszego `socketu`. Zacznijmy od usunięcia plików utworzonych przez skrypt podczas wcześniejszego testowania.

```console
$ sudo rm /tmp/my-logger.fifo
$ sudo rm /tmp/my-logger.log
```

Następnie zainstalujmy `my-logger.socket`.

```console
$ sudo systemctl enable my-logger.socket
Created symlink /etc/systemd/system/sockets.target.wants/my-logger.socket → /etc/systemd/system/my-logger.socket.
```

Jak pamiętamy, instalowanie tworzy jedynie linki symboliczne, więc plik `FIFO` jeszcze nie powstał. Włączmy teraz nasz `socket` i sprawdźmy jego status, a także status `service'u`.

```console
$ sudo systemctl start my-logger.socket
$ systemctl status my-logger.socket
● my-logger.socket - Socket for My Logger
   Loaded: loaded (/etc/systemd/system/my-logger.socket; enabled; vendor preset: enabled)
   Active: active (listening) since Thu 2019-10-03 11:25:32 CEST; 1s ago
   Listen: /tmp/my-logger.fifo (FIFO)
   CGroup: /system.slice/my-logger.socket

paź 03 11:25:32 KOM-135-LINUX systemd[1]: Listening on Socket for My Logger.

$ systemctl status my-logger.service
● my-logger.service - My Logger
   Loaded: loaded (/etc/systemd/system/my-logger.service; static; vendor preset: enabled)
   Active: inactive (dead)
```

A zatem `socket` został uruchomiony, natomiast `service` nadal jest nieaktywny. Gdy jednak wyślemy coś do pliku wymiany danych i sprawdzimy ponownie status `service'u`...

```console
$ echo "Hi!" > /tmp/my-logger.fifo
$ systemctl status my-logger.service
● my-logger.service - My Logger
   Loaded: loaded (/etc/systemd/system/my-logger.service; static; vendor preset: enabled)
   Active: active (running) since Thu 2019-10-03 12:05:35 CEST; 2s ago
 Main PID: 24625 (my-logger.sh)
    Tasks: 1 (limit: 4915)
   CGroup: /system.slice/my-logger.service
           └─24625 /bin/bash /usr/local/bin/my-logger.sh

paź 03 12:05:35 KOM-135-LINUX systemd[1]: Started My Logger.
```

...zobaczymy, że `service` się uruchomił! A zatem aktywacja `socketem` zadziałała! Gdyby z jakichś powodów nasz `service` zakończył swoje działanie, kolejne wysłanie wiadomości do pliku `/tmp/my-logger.fifo` spowoduje jego ponowne uruchomienie. Nie musimy zatem dodawać jawnej zależności innego `service'u` od naszego loggera, ponieważ ten sam zostanie uruchomiony przez `socket`. Prawdziwa modułowość!

Pojawia się jedno ale. Musieliśmy podać zahardkodowaną ścieżkę pliku `FIFO` w definicji `socketu` oraz w skrypcie. Czy jest jakiś sposób, aby podczas inicjalizacji `service'u` odwołać się jakoś do pliku podanego w definicji `socketu`? Otóż tak. W języku C łuży do tego [`sd_listen_fds`](https://www.freedesktop.org/software/systemd/man/sd_listen_fds.html#). Ponieważ jednak piszemy w Bashu, musimy odczytać wartość zmiennej środowiskowej `LISTEN_FDS`, która przechowuje liczbę deskryptorów pliku przekazywanych przez `systemd` do uruchamianego procesu (może być ich więcej niż jeden!). Zakładając, że przekazywany jest tylko jeden deskryptor pliku, możemy zamiast

```bash
read line < ${pipe}
```

zastosować

```bash
read line <&3
```

Istnieje również inne obejście, które wymaga jednak ingerencji w definicję `socketu`.

```ini
[Unit]
Description=Socket for My Logger

[Socket]
ListenFIFO=/tmp/my-logger.fifo
FileDescriptorName=/tmp/my-logger.fifo

[Install]
WantedBy=sockets.target
```

Argument parametru `FileDescriptorName` można odczytać po stronie `service'u` ze zmiennej środowiskowej `LISTEN_FDNAMES`.

```bash
if [[ -z ${LISTEN_FDNAMES} ]]; then
    pipe=${LISTEN_FDNAMES}
else
    pipe=/tmp/my-logger.fifo
fi
```

W ten sposób możemy użyć pliku wymiany danych udostępnionego przez `socket` lub swój domyślny plik, gdyby skrypt zostal uruchomiony z ręki.

Istnieje wiele szczegółów dotyczących aktywacji `socketami`, ale obecnie poprzestańmy na tym i przejdźmy do innego ciekawego zagadnienia - `timerów`.
