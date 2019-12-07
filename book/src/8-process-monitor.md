# Monitorowanie procesu

Gdy piszemy aplikację, która może się zawiesić (innymi słowy: nie mamy zaufania do siebie jako programisty) warto dołożyć do niej mechanizm, który regularnie sprawdza czy działa. W nomenklaturze mikroprocesorów taki mechanizm nazywamy _watchdogiem_. `systemd` umożliwia uruchomienie dla wybranego `service'u` watchdoga, który raz na określony interwał czasowy będzie oczekiwał na sygnał od procesu. Jeżeli go nie otrzyma, zrestartuje usługę.

Zobaczmy jak
