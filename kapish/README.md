Jamie Kirkwin
V00875987 
jkirkwin
CSC 360 Assignent 1 - Kapish Shell

## Info:
* I Used the tutorial linked by Yvonne (https://brennan.io/2015/01/16/write-a-shell-in-c/) as a structural guide.
* I implemented the history functionality and included a test suite for it (make test_history will give you the executable)
* Used http://www.csl.mtu.edu/cs4411.ck/www/NOTES/signal/install.html to help understand signal handling for control-c functionality.
* If user enters control+d partway through a line, it doesn't terminate the program. It should be treated simply as a shortcut for 'exit', not as a signal that can be sent at any time.

## Bugs
* All known bugs fixed