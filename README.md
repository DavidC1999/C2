# Programming language
This purely interpreted programming language goes by the very creative name of "Programming language"

It is a hobby project written in C (kinda regret that choice now, but sunken cost fallacy and all that) to mess around with parsing a programming language. Here's an example of what the syntax looks like as of now:

```
var theLetterD;
var newLine;

func printD() {
	putc(theLetterD);
	putc(newLine);
}

// The main function will be called automatically
func main() {
	newLine = (1 + 1) * 4 + 2;
	theLetterD = newLine + 58;
	printD();
}
```

## TO DO LIST
More or less in order of priority:
- [ ] If statements
- [ ] Local variables
- [ ] While loops
- [ ] User input
- [ ] Function parameter(s) for user-defined functions
- [ ] Return values for functions (currently always implicitly 0)
- [ ] Pointers
- [ ] Arrays
- [ ] String literals
- [ ] Boolean operators high precendence than arithmetic operators