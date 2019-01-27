Jamie Kirkwin
V00875987 
jkirkwin

## Help received:
The only real outside help I received was from the tutorial linked by Yvonne (https://brennan.io/2015/01/16/write-a-shell-in-c/).

## Other info
I implemented the history functionality and included a test suite for it
Used http://www.csl.mtu.edu/cs4411.ck/www/NOTES/signal/install.html to help with my signal handling code

### Bugs
The current working directory string seems to be getting corrupted, or whatever it is that ls accesses to show contents of that directory. 
A set of steps to reproduce:
1. Run kapish
2. ls
3. env
4. history
5. !l
Seems to be caused by using shebang history shortcut.
Sometimes fixes its self after a number of other commands are entered.
cd continues to work as normal
ls shows "cannot access XXX : No such file or directory" where XXX is some string of garbage,
or it shows "Bad address"


if user enters control+d partway through a line, it doesnt terminate the program