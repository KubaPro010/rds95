# RDS95

RDS95 is a ligth software RDS encoder for linux

RDS95 follows the IEC 62106 standard (available at the RDS Forum website)

## Diffrences between IEC 62106 and EN 50067

The newer standard which is the IEC one, removes these features:

- MS
- PIN
- LIC
- DI (partially, only dynamic pty is left)
- EWS (now ODA)
- IH (now ODA)
- RP
- TDC (now ODA)

## Unique features

RDS95 is the only RDS2 encoder to have a independent tunneling feature, where you can pick the group sequence from the RDS2 stream in order to speed up the transmission of RT or LPS (in my tests, this helps a lot)

## Commands

### PS

Sets the Program Service: `PS=* RDS *`

### PI

Sets the PI code: `PI=FFFF`

### TP

Sets the TP flag: `TP=1`

### TA

Sets the TA flag and triggers Traffic PS: `TA=0`  
*May be overridden by EON*

### DPTY

Sets the DPTY flag: `DPTY=1`  
*Formerly DI*

### CT

Toggles the transmission of CT groups: `CT=1`  

### AF

Sets the AF frequencies: `AF=95,89.1`  
Clears the AF: `AF=`  

### TPS

Sets the Traffic PS: `TPS=Traffic!` (default not set)  
*TPS is transmitted instead of PS when TA flag is on*

### RT1

Sets the first radio text: `RT1=Currently Playing: Jessie Ware - Remember Where You Are` or `TEXT=Currently Playing: Jessie Ware - Remember Where You Are`  

### RT2

Sets the second radio text: `RT2=Radio Nova - Best Hits around!`  

### PTY

Sets the programme type flag: `PTY=11`

PTY values are diffrent for RDS and RDBS, look for them online

### ECC

Sets the extended country code: `ECC=E2`  
*Note that the ECC is depended on the first letter of the PI, for example PI:3 and ECC:E2 is poland, but PI:1 would be the czech republic*

### RTP

TODO: RTP

### LPS

Sets the LPS: `LPS=NovaFM❤️`  
*Note that LPS does UTF-8, while PS, RT don't*  

### PTYN

Sets the programme type name: `PTYN=Football`

### AFCH

TODO: AFCH  

### UDG1

Sets the user defined group, max 8 groups: `UDG1=6000FFFFFFFF`  

### UDG2

See [UDG1](#udg1)

### G

Sends a custom group to the next group available: `G=F100FFFFFFFF` or `G=00002000AAAAAAAA` for RDS2

### RT1EN

Enables RT 1: `RT1EN=1`  

### RT2EN

Enables RT 2: `RT1EN=2`  

### RTPER

RT Switching period, in minutes: `RTPER=5`

### LEVEL

Sets the RDS output level: `LEVEL=255`

### RESET

Resets the internal state of the encoder: `RESET`

### PTYNEN

Enables PTYN: `PTYNEN=1`

### GRPSEQ

Sets the group sequence for stream0, available groups:

- 0: 4 PSs
- 1: ECC
- 2: RT
- A: PTYN
- E: EON
- X: UDG1
- Y: UDG2
- R: RT+
- 3: ODA
- F: LPS

`GRPSEQ=002222`

### RDSGEN

Sets the rds generator level:

- 0: No streams
- 1: Stream 0 only
- 2: Stream 0 and 1

`RDSGEN=1`

TODO: Rest of the cmds