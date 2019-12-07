# Czym jest systemctl?

`systemctl` to narzędzie, które służy do ręcznego zarządzania i doglądania sesji `systemd` przy pomocy terminala. Domyślnie `systemctl` odwołuje się do sesji systemowej (jak gdyby wywoływać `systemctl --system`), jednak możemy za jego pomocą kontrolować również sesje użytkownika. W tym celu należy każdorazowo korzystać z `systemctl --user`. Co więcej, `systemctl` pozwala na kontrolowanie sesji zdalnie, na innym urządzeniu za pomocą SSH. Wystarczy użyć `systemctl --host <nazwa-uzytkownika>@<nazwa-hosta>`.

Aby wylistować wszystkie zainstalowane `unity` z podziałem na typy (o różnych typach `unitów` będziemy mówić w kolejnych rozdziałach):

```console
$ systemctl list-units
```

Aby wylistować tylko `unity` wybranego typu:

```console
$ systemctl --type=<nazwa-typu>
```

Aby podejrzeć status całego systemu:

```console
$ systemctl status
```

Aby wyświetlić `unity`, w których występuje błąd:

```console
$ systemctl --failed
```

Aby zrestartować system:

```console
$ sudo systemctl reboot
```

Aby wyłączyć system:

```console
$ sudo systemctl poweroff
```

Aby uśpić system (do RAMu):

```console
$ sudo systemctl suspend
```

Aby zahibernować system (na dysk):

```console
$ sudo systemctl hibernate
```

Aby przejść do trybu `rescue`:

```console
$ sudo systemctl rescue
```

Aby przejść do trybu `emergency`:

```console
$ sudo systemctl emergency
```

Rozróżnienie pomiędzy trybami `rescue` i `emergency` rozjaśnia [ten wpis](https://lists.freedesktop.org/archives/systemd-devel/2010-September/000213.html). Aby przejść do trybu normalnego:

```console
$ sudo systemctl default
```

Istnieje sporo poleceń `systemctl`, dotyczących poszczególnych `unitów`. Będziemy omawiać je na przykładach w dalszych rozdziałach. Przejdźmy zatem do pierwszego własnego `unitu`!
