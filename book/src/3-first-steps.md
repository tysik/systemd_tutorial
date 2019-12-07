# Pierwsze kroki

Istnieje kilka typów `unitów`, a rozróżniać je można m.in. w oparciu o sufiks w nazwie pliku. Podstawowy `unit`, który możemy dodać to tzw. `service`. Pliki definiujące `service` uruchamiają zwykle jakiś proces i mają sufiks... `.service`. Dla przetestowania, napiszmy prosty skrypt, który chcielibyśmy wywołać.

```bash
#!/bin/bash

echo "Hello there!" > /tmp/my-first-service.log
exit 0
```

Skrypt tworzy i/lub nadpisuje zawartość pliku `/tmp/my-first-service.log` słowami `Hello there!`. Zapiszmy go jako `my-first-service.sh`, nadajmy mu tryb wykonywania i sprawdźmy czy działa.

```console
$ chmod +x my-first-service.sh
$ ./my-first-service.sh
$ cat /tmp/my-first-service.log
Hello there!
```

Usuńmy plik tymczasowy, a następnie skopiujmy skrypt do katalogu `/usr/local/bin/`.

```console
$ rm /tmp/my-first-service.log
$ sudo cp my-first-service.sh /usr/local/bin/
```

Teraz zajmijmy się definicją `service'u`. Oto ona:

```ini
[Unit]
Description=My First Service

[Service]
ExecStart=/usr/local/bin/my-first-service.sh
```

Definicja składa się z jednej lub wielu sekcji (tutaj: `[Unit]` oraz `[Service]`) oraz występujących w nich parametrów. Obszerny opis parametrów sekcji `[Unit]` znajduje się [tutaj](https://www.freedesktop.org/software/systemd/man/systemd.unit.html), natomiast parametrów sekcji `[Service]` [tutaj](https://www.freedesktop.org/software/systemd/man/systemd.service.html). Nasz `service` posiada jedynie opis i ścieżkę pliku wykonywalnego, który ma zostać uruchomiony. Technicznie rzecz biorąc, istnienie sekcji `[Unit]` w naszym przykładzie nie jest konieczne, jednak dobrym zwyczajem jest dodanie krótkiego opisu `unitu`.

Zapiszmy definicję jako `my-first-service.service` i skopiujmy ją do `/etc/systemd/system/`.

```console
$ sudo cp my-first-service.service /etc/systemd/system/
```

Możemy teraz sprawdzić jaki jest status naszego `service'u`.

```console
$ systemctl status my-first-service.service
● my-first-service.service - My First Service
   Loaded: loaded (/etc/systemd/system/my-first-service.service; static; vendor preset: enabled)
   Active: inactive (dead)
```

Jeżeli chcemy sprawdzić status `service'u`, nie musimy podawać jego pełnej nazwy, wystarczy nazwa bez sufiksu `.service`, jednak w tym opracowaniu będziemy zawsze podawali pełną nazwę.

Zwrotka, jaką otrzymujemy ma kilka wierszy. Pierwszy wiersz to pełna nazwa `unitu`, po której następuje opis zawarty w sekcji `[Unit]`. Następna linia dotyczy stanu zainstalowania `unitu`. Dowiadujemy się tutaj gdzie znajduje się plik konfiguracyjny, w jakim trybie ten został zainstalowany (słówko `static` - o tym więcej później) oraz czy `unit` nie został domyślnie zablokowany przez dostawcę systemu (`vendor preset: enabled`). Kolejna linia dotyczy stanu aktywności `unitu` - w naszym przypadku `service` jest nieaktywny. W ogólności status pokazuje więcej informacji - będziemy omawiać je na bieżąco.

Spróbujmy teraz uruchomić nasz `service`, a następnie sprawdźmy, czy nasz skrypt zadziałał tak, jak należy.

```console
$ sudo systemctl start my-first-service.service
$ cat /tmp/my-first-service.log
Hello there!
```

Wszystko wygląda dobrze. Zwróć uwagę, że wystartowanie `service'u` (w przeciwieństwie do sprawdzenia statusu) jest modyfikacją `systemd`, więc wywołanie `systemctl start` wymaga uprawnień `roota`. Sprawdźmy ponownie status `service'u`.

```console
$ systemctl status my-first-service.service
● my-first-service.service - My First Service
   Loaded: loaded (/etc/systemd/system/my-first-service.service; static; vendor preset: enabled)
   Active: inactive (dead)

sie 31 08:36:32 KOM-135-LINUX systemd[1]: Started My First Service.
```

Hmm, niby w statusie pojawiła się notka `Started My First Service`, jednak nadal jest on oznaczony jako `inactive`. Dlaczego? Dzieje się tak dlatego, że nasz proces jest bardzo prosty i kończy pracę chwilę po uruchomieniu, a zatem po krótkim stanie `active` przechodzi do `inactive`. Notka o rozpoczęciu `service'u` stanowi skrótowy log, który prezentowany jest w statusie.

## Typ service'u

Każdy `service` posiada w sekcji `[Service]` parametr `Type`, który domyślnie ustawiony jest na wartość `simple`. W przypadku typu `simple`, `service` oznaczany jest jako `active` tuż po utworzeniu przez `systemd` osobnego procesu systemowego (`forka`), w którym uruchomiony zostanie nasz skrypt. Gdy proces kończy pracę, `systemd` analizuje zwracany kod numeryczny i jeśli ten równy jest `0`, proces ponownie wchodzi w stan `inactive`. Zobaczmy co stanie się, jeśli celowo zwrócimy z naszego skryptu niezerowy kod (co w środowisku linuksowym przyjmuje się za informację o błędzie). W tym celu zmodyfikujmy nasz skrypt.

```bash
#!/bin/bash

echo "Hello there!" > /tmp/my-first-service.log
exit 1
```

A następnie skopiujmy go ponownie w odpowiednie miejsce.

```console
$ sudo cp my-first-service.sh /usr/local/bin/
```

Gdy teraz uruchomimy nasz `service` i sprawdzimy jego status, zobaczymy:

```console
$ sudo systemctl start my-first-service.service
$ systemctl status my-first-service.service
● my-first-service.service - My First Service
   Loaded: loaded (/etc/systemd/system/my-first-service.service; static; vendor preset: enabled)
   Active: failed (Result: exit-code) since Thu 2019-08-31 09:05:36 CEST; 1s ago
  Process: 25016 ExecStart=/usr/bin/my-first-service.sh (code=exited, status=1/FAILURE)
 Main PID: 25016 (code=exited, status=1/FAILURE)

sie 31 09:05:36 KOM-135-LINUX systemd[1]: Started My First Service.
sie 31 09:05:36 KOM-135-LINUX systemd[1]: my-first-service.service: Main process exited, code=exited, status=1/FAILURE
sie 31 09:05:36 KOM-135-LINUX systemd[1]: my-first-service.service: Failed with result 'exit-code'.
```

Ze statusu dowiadujemy się, że `service` został uruchomiony z `PID=25016` oraz że jego wywołanie nie powiodło się i w wyniku zwrócono kod błędu `1`.

> Należy baczyć, by wartość zwracana po zakończeniu naszego procesu była równa zeru, jeśli działanie powiodło się i różna od zera w przeciwnym wypadku.

Przywróćmy teraz poprawne działanie naszego skryptu i zastanówmy się nad następującą kwestią. Co, jeśli chcielibyśmy, aby nasz skrypt był wykonany raz, ale po zakończeniu mimo wszystko był oznaczony jako aktywny? Może to być pomocne przy analizie stanu systemu oraz zabezpieczać nas przed wielokrotnym uruchomieniem serwisu (wywołanie `systemctl start` na serwisie, który jest już w stanie `active` nic nie robi). Do tego rodzaju `service'ów` należy skorzystać z kombinacji parametrów `Type=oneshot` oraz `RemainAfterExit=yes`. Wprowadźmy następującą modyfikację.

```ini
[Unit]
Description=My First Service

[Service]
Type=oneshot
RemainAfterExit=yes
ExecStart=/usr/local/bin/my-first-service.sh
```

Zapiszmy plik, wgrajmy go ponownie do właściwego katalogu i dodatkowo zweryfikujmy, czy `systemd` go rozpoznaje.

```console
$ sudo cp my-first-service.service /etc/systemd/system/
$ systemctl cat my-first-service.service
# Warning: my-first-service.service changed on disk, the version systemd has loaded is outdated.
# This output shows the current version of the unit's original fragment and drop-in files.
# If fragments or drop-ins were added or removed, they are not properly reflected in this output.
# Run 'systemctl daemon-reload' to reload units.
# /etc/systemd/system/my-first-service.service
[Unit]
Description=My First Service

[Service]
Type=oneshot
RemainAfterExit=yes
ExecStart=/usr/bin/my-first-service.sh
```

Narzędzie `systemctl cat <nazwa-unitu>` przydaje się do upewnienia się, czy konfiguracja `unitu` nie jest przez coś nadpisywana. W naszym przypadku widzimy, że konfiguracja `service'u` w pliku jest zgodna z naszymi oczekiwaniami, jednak dostaliśmy wyraźne ostrzeżenie, mówiące o tym, że uległa ona zmianie w stosunku do tej, która jest załadowana przez `systemd`. Należy załadować ją ponownie poleceniem `systemctl daemon-reload`. Zróbmy to teraz i ponownie sprawdźmy konfigurację.

```console
$ sudo systemctl daemon-reload
$ systemctl cat my-first-service.service
# /etc/systemd/system/my-first-service.service
[Unit]
Description=My First Service

[Service]
Type=oneshot
RemainAfterExit=yes
ExecStart=/usr/bin/my-first-service.sh
```

Tym razem ostrzeżenie nie zostało wyświetlone. Sprawdźmy również status naszego `service'u`.

```console
$ systemctl status my-first-service.service
● my-first-service.service - My First Service
   Loaded: loaded (/etc/systemd/system/my-first-service.service; static; vendor preset: enabled)
   Active: failed (Result: exit-code) since Thu 2019-08-31 09:05:36 CEST; 27min ago
 Main PID: 25016 (code=exited, status=1/FAILURE)

sie 31 09:05:36 KOM-135-LINUX systemd[1]: Started My First Service.
sie 31 09:05:36 KOM-135-LINUX systemd[1]: my-first-service.service: Main process exited, code=exited, status=1/FAILURE
sie 31 09:05:36 KOM-135-LINUX systemd[1]: my-first-service.service: Failed with result 'exit-code'.
```

Nic się nie zmieniło - utrzymuje się ostatni zapamiętany status. A zatem ponowne załadowanie konfiguracji `unitów` nie dotyka ich dotychczasowego trybu pracy. Zrestartujmy zatem nasz unit i ponownie sprawdźmy status.

```console
$ sudo systemctl restart my-first-service.service
$ systemctl status my-first-service.service
● my-first-service.service - My First Service
   Loaded: loaded (/etc/systemd/system/my-first-service.service; static; vendor preset: enabled)
   Active: active (exited) since Thu 2019-08-31 09:36:32 CEST; 5s ago
  Process: 31700 ExecStart=/usr/bin/my-first-service.sh (code=exited, status=0/SUCCESS)
 Main PID: 31700 (code=exited, status=0/SUCCESS)

sie 31 09:36:32 KOM-135-LINUX systemd[1]: Starting My First Service...
sie 31 09:36:32 KOM-135-LINUX systemd[1]: Started My First Service.
```

W końcu udało nam się osiągnąć faktyczny status `active`. Typ `service'u` `oneshot` ma taką właściwość, że `systemd` oznacza go jako aktywny _po_ jego wykonaniu. Ma to tę zaletę, że w przypadku szeregowania startu poszczególnych `service'ów`, procesy zależne od `service'u` typu `oneshot` faktycznie uruchomią się po nim, a nie równolegle z nim (jak to ma miejsce w przypadku `service'ów` typu `simple`). O zależnościach między `unitami` będzie mowa za chwilę.

No dobrze, ale cały czas uruchamiamy nasz `service` _z ręki_, więc równie dobrze moglibyśmy po prostu uruchamiać bezpośrednio nasz skrypt. Jak sprawić, aby nasz skrypt uruchomił się przy starcie systemu? O tym w następnym rozdziale.
