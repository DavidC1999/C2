# Programming language
This purely interpreted programming language goes by the very creative name of "Programming language"

It is a hobby project written in C (kinda regret that choice now, but sunken cost fallacy and all that) to mess around with parsing a programming language. Here's an example of what the syntax looks like as of now:

```
var i;

func incrementI() {
	i = i + (10 / 10);
}

// The main function will be called automatically
func main() {
	var max;

	i = 0;
	max = input_num();
	
	while(i < max) {
		print(i);

		incrementI();
	}
}

```

## TO DO LIST
More or less in order of priority:
- [ ] Initial value for variables
- [ ] Function parameter(s) for user-defined functions
- [ ] Return values for functions (currently always implicitly 0)
- [ ] Pointers
- [ ] Arrays
- [ ] String literals
- [ ] Boolean operators higher precendence than arithmetic operators