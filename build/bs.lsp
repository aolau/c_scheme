(progn
  (defun mapcar (f l)
    (if l (cons (f (car l))
                (mapcar f (cdr l)))
        nil))
  (defun range (n)
    (if (equal n 0)
        nil
        (cons n (range (- n 1)))))
  (defun reduce (f l a)
    (if l
        (reduce f (cdr l) (f (car l) a))
        a))
  (defun repeat (x n)
    (if (equal n 0)
        nil
        (cons x (repeat x (- n 1)))))
  (defun nth (n l)
    (if (equal n 1)
        (car l)
        (nth (- n 1) (cdr l)))))
