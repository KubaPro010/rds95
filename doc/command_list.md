## MiniRDS command list

Most commands are in the ASCII format, for RT you can use `TEXT` or `RT1`, however some comamnds werent yet transitioned to the new command syntax, and here they are:

### Commands

#### `MPX`
Set volumes in percent modulation for individual MPX subcarrier signals.

`MPX 0,9,9,9,9`

Carriers: (first to last)
Pilot tone
RDS 1
RDS 2 (67 khz)
RDS 2 (71 khz)
RDS 2 (76 khz)

#### `VOL`
Set the output volume in percent.

`VOL 100`

## Other
See src/supported_cmds.txt