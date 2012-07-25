# mruby SQLite extention

## Build

    $ git clone git://github.com/mruby/mruby.git ../mruby
    $ make

## Load

    $ LD_LIBRARY_PATH=. sqlite3
    sqlite> SELECT load_extension('libsqlite3ext_mruby.so.1.0.0');

## Functions

    SELECT mrb_eval('ARGV.shift.to_i * 2', 42);

    SELECT mrb_load('/path/to/library.rb');

    SELECT mrb_load_irep('/path/to/library.mrb');

## Create function by ruby

    $ cat test.rb

    def fib(n)
      n < 2 ? n : fib(n - 1) + fib(n - 2)
    end

    create_function :fib do |n|
      fib(n)
    end

    $ LD_LIBRARY_PATH=. sqlite3
    sqlite> .load libsqlite3ext_mruby.so.1.0.0;
    sqlite> SELECT mrb_load('test.rb');
    sqlite> SELECT fib(30);
    832040
    sqlite>

## License

Copyright (c) 2012 Jiro Nishiguchi

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

## Author

Jiro Nishiguchi <jiro@cpan.org>
