savedcmd_/home/berke/opsystem/lkmmodule.mod := printf '%s\n'   lkmmodule.o | awk '!x[$$0]++ { print("/home/berke/opsystem/"$$0) }' > /home/berke/opsystem/lkmmodule.mod
