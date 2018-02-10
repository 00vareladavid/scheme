(eq? 1 (if #t 1 2))
(eq? 2 (if #f 1 2))
(eq? 1 (if #t 1))
(eq? undef (if #f 1))
(eqv? (car xs) (car xs)) ;should be the same memory location

; test predicates: number, symbol, procedure, boolean, pair

((lambda (x) (+ x 1)) 2)
; also make sure that dynamic scoping does not take place


