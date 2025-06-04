# MESSAGE FORMAT

#### Version 0

V --- T --- L --- L --- \[M\]

BYTE    |   DESCRIPTION
| :---: |   :---:
L       |   Message Length
M       |   Message Body Byte(s)
T       |   Message Type
V       |   Version

# MESSAGE TYPES

## CLIENT MESSAGES

### START/CONTINUE \[0\]

#### Version 0 ->

NO BODY

### PAUSE \[1\]

#### Version 0 ->

NO BODY

### MOVE PADDLE \[2\]

#### Version 0 ->

D

BYTE    |   DESCRIPTION
| :---: |   :---:
D       |   Move Direction

<br>

DIRECTION   |   VALUE
| :---:     |   :---:
Up          |   0x1
Down        |   0x2
Stop        |   0x0

### QUIT \[3\]

#### Version 1

NO BODY

## SERVER MESSAGES

### GAME INIT \[0\]

#### Version 0 ->

2 x 1 --- 2 x 2

BYTE    |   DESCRIPTION
| :---: |   :---:
1       |   Player 1 Port
2       |   Player 2 Port

### GAME PAUSED \[1\]

#### Version 0 ->

NO BODY

### GAME OVER \[2\]

#### Version 0

P

BYTE    |   DESCRIPTION
| :---: |   :---:
P       |   Winners Player ID

#### Version 1

P --- F --- 2 x S

BYTE    |   DESCIPTION
| :---: |   :---:
F       |   Finishing Status
P       |   Actors Player ID
S       |   Final Score

FINISHING STATUS    |   VALUE
| :---:             |   :---:
Actor Won           |   0x1
Actor Quit          |   0x2
Server Closed       |   0x3

### STATE UPDATE \[3\]

#### Version 0 ->

4 x 1 --- 4 x 2 --- 8 x B --- 2 x S

BYTE    |   DESCRIPTION
| :---: |   :---:
1       |   Player 1 Paddle Position
2       |   Player 2 Paddle Position
B       |   Ball Position
S       |   Score
