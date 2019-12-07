# Zdarzenia czasowe

Czasami chcielibyśmy uruchomić jakiś proces z opóźnieniem lub uruchamiać go cyklicznie. Do tego celu służą w `systemd` `timery`. `timer` to kolejny typ `unitu`, który pozwala skonfigurować _co_ i _kiedy_ ma się wykonać. Przeanalizujmy następujący przykład. Chcielibyśmy raz na minutę zapisywać poziom użycia naszej pamięci oraz dysku, a także czas i datę tego zapisu. Skorzystamy w tym celu z następującego skryptu.

```bash
#!/bin/bash

outputFile=/tmp/my-memory-monitor.log

currentDate=$(date "+%H:%M:%S %d-%m-%Y")
read _ totalMem _ freeMem _< <(free -hw | head -2 | tail -1)
read _ totalDisk _ freeDisk _< <(df -h /dev/sda1 | head -2 | tail -1)

printf "[%s]\t" "${currentDate}" >> ${outputFile}
printf "Memory (free/total): %s/%s\t" "${freeMem}" "${totalMem}" >> ${outputFile}
printf "Disk (free/total): %s/%s\n" "${freeDisk}" "${totalDisk}" >> ${outputFile}

exit 0
```

Skrypt ten odczytuje wspomniane wartości i zapisuje je w postaci jednej linijki w pliku `outputFile`. Zapiszmy skrypt pod nazwą `my-memory-monitor.sh`, nadajmy mu odpowiednie uprawnienia i skopiujmy w standardowe miejsce.

```console
$ chmod +x my-memory-monitor.sh
$ sudo cp my-memory-monitor.sh /usr/local/bin/
```

Teraz utwórzmy odpowiedni `service`. (Podobnie jak `socket`, `timer` nie może bezpośrednio wywołać skryptu, a jedynie aktywować inny `unit`.)

```ini
[Unit]
Description=My Memory Monitor

[Service]
ExecStart=/usr/local/bin/my-memory-monitor.sh
```

Zapiszmy definicję pod nazwą `my-memory-monitor.service` i skopiujmy ją do odpowiedniego katalogu.

```console
$ sudo cp my-memory-monitor.service /etc/systemd/system/
```

Następnie przejdźmy do zapisania definicji `timera`.

```ini
[Unit]
Description=Timer for My Memory Monitor (once every minute)

[Timer]
OnActiveSec=0 seconds
OnUnitActiveSec=1 minute

[Install]
WantedBy=timers.target
```

W powyższej definicji pojawiła się nowa sekcja `[Timer]`, w której konfigurujemy zdarzenia czasowe. Parametr `OnActiveSec` definiuje czas odpalenia `unitu` względem chwili uruchomienia `timera`. Parametr `OnUnitActiveSec` definiuje czas odpalenia `unitu` względem jego ostatniej aktywacji. Warto zwrócić uwagę, że sam parametr `OnUnitActiveSec` nie uruchomi docelowego `unitu` dopóty, dopóki ktoś inny nie uruchomi go po raz pierwszy. Szczegółowy opis parametrów sekcji `[Timer]` znajduje się w [dokumentacji](https://www.freedesktop.org/software/systemd/man/systemd.timer.html). Ponadto, opis jednostek czasu rozumianych przez `systemd` znajduje się [tutaj](https://www.freedesktop.org/software/systemd/man/systemd.time.html#).

W sekcji `[Install]` podpięliśmy nasz `timer` pod `timers.target`, który jest standardowym punktem zbiorczym dla `timerów` podczas startu systemu. Nie jest on wymagany, ale warto o nim pamiętać. Podobnie jak w przypadku `socketów`, `timer` domyślnie uruchamia `service` o takiej samej nazwie. Gdybyśmy chcieli wskazać inny `unit` do uruchomienia, możemy posłużyć się parametrem `Unit`.

Zapiszmy nasz `timer` pod nazwą `my-memory-monitor.timer` i skopiujmy plik w odpowiednie miejsce.

```console
$ sudo cp my-memory-monitor.timer /etc/systemd/system/
```

Czas na szybki test. W pierwszej kolejności zainstalujmy i włączmy nasz `timer`, po czym sprawdźmy jego status.

```console
$ sudo systemctl enable my-memory-monitor.timer
Created symlink /etc/systemd/system/timers.target.wants/my-memory-monitor.timer → /etc/systemd/system/my-memory-monitor.timer.

$ sudo systemctl start my-memory-monitor.timer
$ systemctl status my-memory-monitor.timer
● my-memory-monitor.timer - Timer for My Memory Monitor (once every minute)
   Loaded: loaded (/etc/systemd/system/my-memory-monitor.timer; disabled; vendor preset: enabled)
   Active: active (waiting) since Fri 2019-10-04 12:10:07 CEST; 12s ago
  Trigger: Fri 2019-10-04 12:11:07 CEST; 47s left

paź 04 12:10:07 KOM-135-LINUX systemd[1]: Started Timer for My Memory Monitor (once every minute).
```

Ze statusu dowiadujemy się, że `timer` działa, a także, że następne odpalenie nastąpi za 47 sekund. Sprawdźmy również status `service'u`.

```console
$ systemctl status memory-monitor.service
● memory-monitor.service - My Memory Monitor
   Loaded: loaded (/etc/systemd/system/memory-monitor.service; static; vendor preset: enabled)
   Active: inactive (dead) since Fri 2019-10-04 12:10:07 CEST; 12s ago
  Process: 7711 ExecStart=/usr/local/bin/my-memory-monitor.sh (code=exited, status=0/SUCCESS)
 Main PID: 7711 (code=exited, status=0/SUCCESS)

paź 04 12:10:07 KOM-135-LINUX systemd[1]: Started My Memory Monitor.
```

Również `service` się aktywował. Warto zwrócić uwagę, że gdyby nasz `service` miał ustawiony parametr `RemainAfterExit=yes`, wówczas nie byłoby możliwe jego cykliczne uruchamianie. Gdy po jakimś czasie sprawdzimy zebrane logi, zobaczymy:

```console
$ cat /tmp/memory-monitor.log
[14:07:24 14-10-2019]	Memory (free/total): 5,5G/15G	Disk (free/total): 177G/229G
[14:08:29 14-10-2019]	Memory (free/total): 5,3G/15G	Disk (free/total): 177G/229G
[14:09:41 14-10-2019]	Memory (free/total): 5,3G/15G	Disk (free/total): 177G/229G
[14:10:57 14-10-2019]	Memory (free/total): 5,9G/15G	Disk (free/total): 177G/229G
```

Warto zwrócić uwagę, że rozrzut czasu jest dość znaczący. Możemy poprawić precyzję odpalenia `timera` korzystając z parametru `AccuracySec`.

Jeżeli chcemy zaprojektować zdarzenie czasowe, które odpali się raz konkretnego dnia, o konkretnej godzinie, powinniśmy skorzystać z parametru `OnCalendar`. Przykładowo, gdybyśmy chcieli wykonać jakieś działanie 1 stycznia 2020 roku o godzinie 00:00, możemy użyć `OnCalendar=2020-01-01 00:00:00`. Gdyby przypadkiem komputer był wówczas akurat wyłączony, a chcielibyśmy upewnić się, że zdarzenie się wykona, warto posłużyć się parametrem `Persistent=true`. W tej konfiguracji `timer` odpali się gdy tylko ponownie włączymy komputer.

Jeżeli przeszło Ci przez myśl, żeby zaprzęgnąć `timery` do regularnego sprawdzania, czy inny proces działa poprawnie - zaniech jej. Przechodzimy teraz do omówienia obsługi watchdoga w `systemd`.
