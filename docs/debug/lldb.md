# lldb

```bash



lldb ./imgclass_test
run
bt

(lldb) run  # run debugger
(lldb) quit # quit debugger

# Examine Once Crash Happens

(lldb) frame variable # or fv : Displays local variables in the current stack frame.
(lldb) print *this    # non static contexts
(lldb) print variable_name

# frame 2 calls fr1 , f1 calls f0 , etc.
(lldb) bt  # get backtrace of execution
frame select 0 # shows top stack frame
register read x8 # from stack frame - check memory addr read



(lldb) bt

# print <variable_name> or p <variable_name>: Prints the value of a variable.

(lldb) print variable_name

## Example Output for Debugging

- Shows us the top frame of the stack

frame select 0

# output : ->  0x10000ab10 <+120>: ldr    x8, [x8, #0x8]

- Then view the register at address x8

register read x8

# output x8 = 0x0000000000000000 => NULL MEMORY ADDRESS

quit


```
