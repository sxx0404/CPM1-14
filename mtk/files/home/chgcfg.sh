#!/bin/sh
ValName1="ServerIP"
ValName2="ServerPort"
ValName3="RecdIP"
ValName4="RecdPort"
for i in `seq 4`; do
	eval ValName=\$ValName$i
	NameLen=`expr length $ValName==`
	eval 'OldVal=`sed -n "/^$ValName=/p" /home/acs09.ini | cut -c $NameLen-`'
	printf "Will you change %s=%s\n[N/y]" $ValName $OldVal
	read NewVal
	if [ "$NewVal" = "Y" -o "$NewVal" = "y" ]; then
		printf "%s=" $ValName
		read NewVal
		eval 'sed -i "s/^$ValName=.*/$ValName=$NewVal/" /home/acs09.ini'
		printf "%s is set to %s\n" $ValName $NewVal
	fi
done

