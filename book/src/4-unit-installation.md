# Instalowanie unitów

Chcielibyśmy, aby nasz `unit` był wywoływany automatycznie wraz ze startem systemu. Pomijając chwilowo definicję _startu systemu_, dodajmy do naszego `service'u` następującą zmianę:

```ini
[Unit]
Description=My First Service

[Service]
Type=oneshot
RemainAfterExit=yes
ExecStart=/usr/local/bin/my-first-service.sh

[Install]
WantedBy=default.target
```

Zanim przejdziemy do bardziej szczegółowego opisu zobaczmy te zmiany w akcji.

```console
$ sudo cp my-first-service.service /etc/systemd/system/
$ sudo systemctl daemon-reload
$ systemctl status my-first-service.service
● my-first-service.service - My First Service
   Loaded: loaded (/etc/systemd/system/my-first-service.service; disabled; vendor preset: enabled)
   Active: active (exited) since Thu 2019-08-31 09:36:32 CEST; 1h 10min ago
 Main PID: 31700 (code=exited, status=0/SUCCESS)
    Tasks: 0 (limit: 4915)
   CGroup: /system.slice/my-first-service.service

sie 31 09:36:32 KOM-135-LINUX systemd[1]: Starting My First Service...
sie 31 09:36:32 KOM-135-LINUX systemd[1]: Started My First Service.
```

Na pierwszy rzut oka wszystko wydaje się takie samo jak poprzednio, jednak po bardziej wnikliwej analizie zauważamy, że słówko `static` zamieniło się na `disabled`. Oznacza to, że nasz `service` nie jest już statyczny, tj. uruchamiany jedynie ręcznie lub przez inne `unity`, lecz ma sekcję `[Install]` i można go trwale zainstalować! Zróbmy to teraz.

```console
$ sudo systemctl enable my-first-service.service
Created symlink /etc/systemd/system/default.target.wants/my-first-service.service → /etc/systemd/system/my-first-service.service.
```

Za chwilę skomentujemy co się tutaj stało, ale najpierw zobaczmy status:

```console
$ systemctl status my-first-service.service
● my-first-service.service - My First Service
   Loaded: loaded (/etc/systemd/system/my-first-service.service; enabled; vendor preset: enabled)
   Active: active (exited) since Thu 2019-08-31 09:36:32 CEST; 1h 36min ago
 Main PID: 31700 (code=exited, status=0/SUCCESS)
    Tasks: 0 (limit: 4915)
   CGroup: /system.slice/my-first-service.service

sie 31 09:36:32 KOM-135-LINUX systemd[1]: Starting My First Service...
sie 31 09:36:32 KOM-135-LINUX systemd[1]: Started My First Service.
```

Słówko `disabled` zmieniło się na `enabled`! Nasz unit został zainstalowany. Od teraz będzie automatycznie uruchamiany po restarcie systemu! Przekonaj się o tym wywołując `sudo systemctl reboot` i sprawdzając zawartość pliku `/tmp/my-first-service.log` po ponownym uruchomieniu.

A teraz odpowiedzmy sobie na pytanie: co tutaj zaszło? `unity` `systemd` mogą mieć w swej konfiguracji zaszyte zależności od innych `unitów`. Przykładowo, w sekcji `[Unit]` `unitu` `X` możemy dodać parametr `Wants` (`Requires`), który przyjmuje jako argument listę nazw innych `unitów`, które są pożądane (bezwzględnie wymagane) do działania tego unitu. Dzięki temu `systemd`, gdy przejdzie do uruchamiania `unitu` `X`, sprawdzi, czy wszystkie inne `unity` wyróżnione w tych parametrach zostały już uruchomione, a jeżeli nie, to uruchomi je równolegle z naszym `unitem`. (Nawiasem mówiąc, równica pomiędzy `Wants`, a `Requires` jest taka, że jeżeli którykolwiek z `unitów` wymienionych w `Requires` zostanie jawnie zatrzymany, to w wyniku `systemd` wyłączy również nasz `unit` `X`, natomiast jeżeli którykolwiek z `unitów` wymienionych w `Wants` zostanie jawnie zatrzymany, to z naszym `unitem` `X` nic się nie stanie.)

No dobrze, ale przecież nie dodaliśmy takich parametrów do definicji naszego `service'u`. I bardzo dobrze! Dodawanie jawnych zależności między `unitami` to w ogólności anti-pattern. Chcemy, żeby nasz system był modułowy, a `unity` działały niezależnie. Nasz `service` nie ma żadnych zależności. Jednak poprzez sekcję `[Install]` dodaliśmy odwrotną zależność - parametr `WantedBy` (`RequiredBy`) przyjmuje jako argument listę `unitów`, które pożądają (bezwzględnie wymagają) naszego `unitu` do działania. (Listę parametrów sekcji `[Install]` można znaleźć w [dokumentacji](https://www.freedesktop.org/software/systemd/man/systemd.unit.html#%5BInstall%5D%20Section%20Options).) Można pomyśleć, że to szaleństwo, aby implikować na innych `unitach` zależność od nas - przecież one wiedzą czego potrzebują do pracy! Okazuje się, że jest to bardzo przydatna cecha `systemd`, szczególnie w połączeniu z `targetami`.

## Grupowanie unitów: targety

`targety` to innego rodzaju `unity`, które nie niosą żadnej funkcjonalności same z siebie - są jedynie pewnego rodzaju punktami zbiorczymi, pod które mogą podpinać się inne `unity`. `systemd` udostępnia szereg Dla dociekliwych - warto zapoznać się z listą specjalnych `unitów` na [specjalnych targetów](https://www.freedesktop.org/software/systemd/man/systemd.special.html), dzięki którym konfiguracje na różnych systemach są zbliżone. Przykładowo `network.target` grupuje `unity` służące do zarządzania siecią. W naszym przypadku skorzystaliśmy z `default.target`, który jest domyślnym `targetem` uruchamianym przez `systemd` przy starcie systemu. Podpinając się pod ten `target` zapewniamy, że nasz `service` zostanie wzięty pod uwagę przy starcie całości.

No dobra, ale o co chodzi z tym linkiem symbolicznym, który powstał gdy wywołaliśmy `systemctl enable my-first-service.service`? Podczas wczytywania konfiguracji `unitów`, `systemd` przeszukuje nie tylko katalogi, które wymieniliśmy w pierwszym rozdziale, ale także dla każdego napotkanego `unitu` sprawdza, czy istnieją podkatalogi `<nazwa-unitu>.wants/`, `<nazwa-unitu>.requires/` oraz `<nazwa-unitu>.d/`. Jeżeli w pierwszych dwóch wymienionych podkatalogach `unitu` `X` znajdują się linki symboliczne do innych `unitów`, `systemd` automatycznie dodaje `unitowi` `X` zależność (odpowiednio `Wants` lub `Requires`) od tych innych `unitów`. I to właśnie zadziałało w przypadku naszego `default.target` - dodaliśmy do niego zależność `Wants`, dzięki której nasz `service` zostanie uruchomiony przy starcie, a jednocześnie w przypadku zatrzymania, nie zakończy działania całego systemu (jak stałoby się w przypadku zależności `Requires`).

Pozostała jeszcze jedna kwestia: definicja startu systemu, którą celowo wcześniej pominęliśmy. Na `default.target` składa się pełnoprawnie działający system. Po drodze są jednak inne punkty, które muszą być spełnione do jego działania. Są to m.in. `basic.target` i `sysinit.target`. Gdybyśmy projektowali swoją własną dystrybucję Linuksa wykorzystującą `systemd`, moglibyśmy zdefiniować własny `target` (np. `our-system-startup.target`) i podpiąć nasz `unit` do niego. Daje to dużą elastyczność przy definiowaniu tego co rozumiemy przez start systemu.

## Uruchamianie, instalacja i maskowanie unitów

Podsumujmy kilka ważnych informacji. Wiemy już jak uruchomić wybrany `unit`:

```console
$ sudo systemctl start <nazwa-unitu>
```

Analogicznie, możemy zatrzymać go poprzez:

```console
$ sudo systemctl stop <nazwa-unitu>
```

Możemy to robić bez względu na to, czy `unit` jest zainstalowany czy nie - ważne, żeby `systemd` potrafił zlokalizować odpowiedni plik. Aby zainstalować dany `unit`, należy zapewnić, że ma on poprawną sekcję `[Install]`, a następnie użyć:

```console
$ sudo systemctl enable <nazwa-unitu>
```

Analogicznie, deinstalacja to po prostu:

```console
$ sudo systemctl disable <nazwa-unitu>
```

Jeżeli `unit` jest już uruchomiony, instalacja lub deinstalacja nie zakłócają jego pracy. Działania te jedynie tworzą lub usuwają odpowiednie linki symboliczne w podkatalogach wskazanych `unitów`.

Istnieje jeszcze jedna forma _wyłączania_ `unitów`: maskowanie. Maskowanie polega na utworzeniu linku symbolicznego w katalogu `/etc/systemd/system/`, o nazwie tożsamej z wybranym `unitem` i wskazującego na `/dev/null`. Ponieważ link jest tworzony w katalogu `/etc/systemd/system/`, maskowanie stosuje się jedynie do `unitów` znajdujących się w innych lokalizacjach (np. `/usr/lib/systemd/system/`).

```console
$ sudo systemctl mask <nazwa-unitu>
Created symlink /etc/systemd/system/<nazwa-unitu> → /dev/null.
```

Analogicznie, odmaskowanie to po prostu:

```console
$ sudo systemctl unmask <nazwa-unitu>
Removed /etc/systemd/system/<nazwa-unitu>.
```

Co istotne, zamaskowanie działającego już `unitu` nie przerywa jego pracy. Dla nabycia wprawy wykonajmy następujący przykład. W pierwszej kolejności wyłączmy i odinstalujmy nasz `service`.

```console
$ sudo systemctl stop my-first-service.service
$ sudo systemctl disable my-first-service.service
Removed /etc/systemd/system/default.target.wants/my-first-service.service.
```

Następnie przenieśmy definicję naszego `service'u` do katalogu `/usr/lib/systemd/system/` (w Ubuntu `/lib/systemd/system/`), po czym przeładujmy ustawienia `systemd`.

```console
$ sudo mv /etc/systemd/system/my-first-service.service /usr/lib/systemd/system/
$ sudo systemctl daemon-reload
```

Teraz ponownie zainstalujmy i włączmy nasz `service`.

```console
$ sudo systemctl enable my-first-service.service
Created symlink /etc/systemd/system/default.target.wants/my-first-service.service → /usr/lib/systemd/system/my-first-service.service.
$ sudo systemctl start my-first-service.service
```

Dla pewności sprawdźmy jego status.

```console
$ systemctl status my-first-service.service
● my-first-service.service - My First Service
   Loaded: loaded (/usr/lib/systemd/system/my-first-service.service; enabled; vendor preset: enabled)
   Active: active (exited) since Tue 2019-10-01 08:52:28 CEST; 6s ago
  Process: 24208 ExecStart=/usr/local/bin/my-first-service.sh (code=exited, status=0/SUCCESS)
 Main PID: 24208 (code=exited, status=0/SUCCESS)

paź 01 08:52:28 KOM-135-LINUX systemd[1]: Starting My First Service...
paź 01 08:52:28 KOM-135-LINUX systemd[1]: Started My First Service.
```

Wszystko działa poprawnie. Teraz zamaskujmy nasz `service` i ponownie sprawdźmy status.

```console
$ sudo systemctl mask my-first-service.service
Created symlink /etc/systemd/system/my-first-service.service → /dev/null.
$ systemctl status my-first-service.service
● my-first-service.service
   Loaded: masked (/dev/null; bad)
   Active: active (exited) since Tue 2019-10-01 08:52:28 CEST; 2min 31s ago
 Main PID: 24208 (code=exited, status=0/SUCCESS)
    Tasks: 0 (limit: 4915)
   CGroup: /my-first-service.service

paź 01 08:52:28 KOM-135-LINUX systemd[1]: Starting My First Service...
paź 01 08:52:28 KOM-135-LINUX systemd[1]: Started My First Service.
```

Jak widać, `service` pozostał aktywny, pomimo że jest zamaskowany. Jedyne co możemy zrobić z takim `unitem` to wyłączyć go lub odmaskować. Ponowne włączenie lub próba instalacji nie powiedzie się. Teraz posprzątajmy po sobie i przejdźmy do innego przykładu.

```console
$ sudo systemctl unmask my-first-service.service
Removed /etc/systemd/system/my-first-service.service.
$ sudo systemctl disable my-first-service.service
$ sudo systemctl stop my-first-service.service
$ sudo rm /lib/systemd/system/my-first-service.service
$ sudo rm /usr/local/bin/my-first-service.sh
$ sudo systemctl daemon-reload
```
