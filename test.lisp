(eq? 1 (if #t 1 2))
(eq? 2 (if #f 1 2))
(eq? 1 (if #t 1))
(eq? undef (if #f 1))

; test predicates: number, symbol, procedure, boolean, pair
