def fib(n)
    n < 2 ? n : fib(n - 1) + fib(n - 2)
end

create_function :fib do |n|
  fib(n)
end

create_function :pi do
  Math::PI
end

create_function :fact do |n|
  (1..n).inject(1) {|s,n| s *= n }
end
