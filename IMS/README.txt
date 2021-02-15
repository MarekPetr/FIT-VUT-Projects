Pre spustenie simulácie na merlinovi je potreba prihlásiť sa s parametrom -X, po prihlásení by sa mal úspešne spustiť Xquartz na lokálnom stroji.

príklad: ssh -X xlogin00@merlin.fit.vutbr.cz

preklad programu prebieha použitím "make all"

spustenie experimentov
make run

spustenie simulácie bez použitých protiopatrení.
./simulator

spustenie simulácie s použitým protiopatrením - Hluboká orba
./simulator 2

spustenie simulácie s použitým protiopatrením - Mělka orba
./simulator 3

spustenie simulácie s použitým protiopatrením - Chemická látka Stutox II
./simulator 4

