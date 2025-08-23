# TypiT
`TypiT` is a TUI single-player typing test application based on `ncurses`.

### How to run
After cloning the repository, you can run the `build.sh` script in the project folder. This will generate the executable, named `typit`.

For a better experience, I suggest to maximize the terminal screen :)

### Customization
In the code, there are two macros that you can overwrite in order to customize the tests:
- `TESTDURATION`, which by default is 30 (seconds)
- `LISTPATH`, which by default is `words.txt`, the word file in the project folder.

You can read how to tweak these values running `./build.sh [-h | --help]`

**NOTE**: if you want to change `LISTPATH`, your word file should match the following:
- only a word for each line of the file
- each word cannot be bigger than `LINEMAXLEN` (if you want, you can change the value of the macro yourself)
