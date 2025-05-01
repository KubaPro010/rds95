# RDS95

(eRT)(RDS2)

RDS95 is a light software RDS encoder for linux

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

RDS95 is the only (as far as i can tell) encoder to transmit the 9-bit AF codes

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

### CT

Toggles the transmission of CT groups: `CT=1`  

### AF

Sets the AF frequencies: `AF=95,89.1`  
Clear the AF: `AF=`  

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

### ERT

Sets the ERT: `ERT=Currently on air we're playing: Lady Gaga - Applause`
*Note that ERT is a 128-character version of RT with UTF-8 support*

### AFO

Sets the AF frequencies for the ODA 9-bit version which enables AF for 64.1-88 MHz: `AFO=69.8,95.0,225` (LowerFM,FM,LF not sure if this even works)
Clear the AFO: `AFO=`

### TEXT

Alias for [RT1](#rt1)

### PTYN

Sets the programme type name: `PTYN=Football`

### DPTY

*Formerly DI*
Sets the DPTY flag: `DPTY=1`  

### SLCD

The 1A group where ECC is sent can also be used to send broadcaster data: `SLCD=FFF`

### ERTP

This only will work if ERT is no longer than 64 characters
See [RTP](#rtp)

### LINK

Toggles the linkage bit in 1A groups, enable this if you have EON about a station and that station has EON about you: `LINK=1`

### SITE

Sets up to 2 site addresses: `SITE=44,95`

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

### PTYNEN

Enables PTYN transmission: `PTYEN=1`

### RTPRUN

Sets the RTP Running bit, to signal if the RTP data is accurate: `RTPRUN=1`

You can also toggle the state: `RTPRUN=1,1`

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
- P: eRT+
- S: ERT
- 3: ODA
- F: LPS
- T: Fast tuning info
- U: ODA AF

`GRPSEQ=002222`

### RTTYPE

Sets the RT1/RT2 types of A/B:

- 0: Set to A
- 1: RT1 is A, RT2 is B
- 2: Default, just toggle A/B

### PROGRAM

Switches the current program, so diffrent saves: `PROGRAM=1`

### RDS2MOD

Sets the RDS2 operation mode:

- 0: Default, full tunnelling of stream 0
- 1: Independent tunelling, RDS2 runs a seperate group sequence

### GRPSEQ2

The Group Sequence for the RDS2 independent tunnelling mode
See [GRPSEQ](#grpseq)

### DTTMOUT

Default text timeout, once runs out it sets the RT1 which is saved in memory: `DTTMOUT=60` (1 hour)

### ERTPRUN

See [RTPRUN](#rtprun)

### INIT

Resets program to default settings, no arguments: `INIT`

### VER

If you have output, then it shows the version of the encoder

### RESET

Resets the internal state: `RESET`

### EONxEN

Enables the EON of x: `EON1EN=1`

### EONxPI

Sets the PI of EON x: `EON1PI=30FE`

### EONxPS

Sets the PS of EON x: `EON1PS=AFERA`

### EONxPTY

Sets the PTY of EON x: `EON1PTY=11`

### EONxTA

Enables the TA of EON x: `EON1TA=1`

### EONxTP

Sets the TP of EON x: `EON1TP=1`

### EONxAF

Sets the AF of EON x: `EON1AF=98.6,95.0`

### EONxDT

Sets the broadcaster data of EON x: `EON1DT=F`

### UDG1

Sets the user defined group, max 8 groups: `UDG1=6000FFFFFFFF`  

### UDG2

See [UDG1](#udg1)

### 2UDG1

Sets the UDG1 of RDS2, max 8 groups expects 4 blocks: `2UDG1=0000200020202020`

### 2UDG2

See [2UDG1](#2udg1)

### RDSGEN

Sets the rds generator level:

- 0: No streams
- 1: Stream 0 only
- 2: Stream 0 and 1

`RDSGEN=1`

## Disclaimer

RDS95 is based on [Anthony96922](https://github.com/Anthony96922/)'s [MiniRDS](https://github.com/Anthony96922/MiniRDS)
