# vim:fileencoding=utf-8
require 'test/unit'
require 'sqlite3'

class TestSQLite3mruby < Test::Unit::TestCase
  def setup
    @db = SQLite3::Database.new(':memory:')
    @db.enable_load_extension(true)
    assert_nothing_raised do
      @db.execute('SELECT load_extension("libsqlite3ext_mruby.so.1.0.0")')
    end
  end

  def teardown
    @db.close
  end

  def test_eval
    assert_equal 2, @db.get_first_value('SELECT mrb_eval("1 + 1")')

    assert_equal 'Array', @db.get_first_value('SELECT mrb_eval("ARGV.class", 1, 2.0, "foo", NULL)')

    assert_equal 4, @db.get_first_value('SELECT mrb_eval("ARGV.size", 1, 2.0, "foo", NULL)')

    assert_equal 'Fixnum', @db.get_first_value('SELECT mrb_eval("ARGV[0].class", 1, 2.0, "foo", NULL)')

    assert_equal 'Float', @db.get_first_value('SELECT mrb_eval("ARGV[1].class", 1, 2.0, "foo", NULL)')

    assert_equal 'String', @db.get_first_value('SELECT mrb_eval("ARGV[2].class", 1, 2.0, "foo", NULL)')

    assert_equal 'NilClass', @db.get_first_value('SELECT mrb_eval("ARGV[3].class", 1, 2.0, "foo", NULL)')

    assert_equal 1, @db.get_first_value('SELECT mrb_eval("true")')

    assert_equal 0, @db.get_first_value('SELECT mrb_eval("false")')

    assert_equal nil, @db.get_first_value('SELECT mrb_eval("nil")')

    assert_equal 'str', @db.get_first_value('SELECT mrb_eval("%q[str]")')

    assert_equal 'sym', @db.get_first_value('SELECT mrb_eval(":sym")')

    assert_equal '[1, 2, 3]', @db.get_first_value('SELECT mrb_eval("[1,2,3]")')

    assert_equal 'foobar', @db.get_first_value('SELECT mrb_eval(?)', <<-END_CODE)
      o = Object.new
      def o.to_s
        'foobar'
      end
      o
    END_CODE
  end

  def test_load
    assert_equal nil, @db.get_first_value('SELECT mrb_load(?)', 'test/lib/global.rb')

    assert_equal 'foobar', @db.get_first_value('SELECT mrb_eval("$foobar")')

    assert_equal nil, @db.get_first_value('SELECT mrb_eval("$foobar = nil")')

    assert_equal nil, @db.get_first_value('SELECT mrb_load_irep(?)', 'test/lib/global.mrb')

    assert_equal 'foobar', @db.get_first_value('SELECT mrb_eval("$foobar")')
  end

  def test_function
    assert_not_nil @db.get_first_value('SELECT mrb_eval(?)', <<-END_CODE)
      def fib(n)
        n < 2 ? n : fib(n - 1) + fib(n - 2)
      end
      create_function(:fib) do |n|
        fib(n)
      end
    END_CODE

    assert_equal 55, @db.get_first_value('SELECT fib(10)')
  end
end
