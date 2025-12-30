; sbcl --script birthday.lisp
 

(defconstant +DAYS-IN-YEAR+ 365) 

(defconstant +PEOPLE+ 24) 

(defconstant +TOTAL-SIMULATIONS+ 1000000) 

(defconstant +NUM-THREADS+ 768) 

(defconstant +MULTIPLIER+ 1664525) 

(defconstant +INCREMENT+ 1013904223) 

(defun simulate 
  (simulations thread-id) 
  (declare 
    (optimize (speed 3) (safety 0) (debug 0))) 
  (let* 
    ( 
      (simulations-per-thread 
        (floor simulations +NUM-THREADS+)) 
      (seed 
        (get-internal-real-time)) 
      (state 
        (logand 
          (logxor seed thread-id) #xFFFFFFFF)) 
      (local-success-count 0) 
      (birthdays 
        (make-array +DAYS-IN-YEAR+ :element-type '(unsigned-byte 8)))) 
  (declare 
    (type (unsigned-byte 32) simulations-per-thread state local-success-count)) 
  (declare 
    (type (unsigned-byte 64) seed)) 
  (loop repeat simulations-per-thread do (fill birthdays 0) 
    (loop repeat +PEOPLE+ do 
      (setf state 
        (logand 
          (+ 
            (* state +MULTIPLIER+) +INCREMENT+) #xFFFFFFFF)) 
      (let 
        ( 
          (birthday 
            (rem state +DAYS-IN-YEAR+))) 
        (incf 
          (aref birthdays birthday)))) 
    (let 
      ((exactly-two-count 0)) 
      (loop for count across birthdays do 
        (if (= count 2) 
          (incf exactly-two-count))) 
      (if 
        (= exactly-two-count 1) 
        (incf local-success-count)))) local-success-count)) 

(defun main () 
  (let* 
    ( 
      (start-time 
        (get-internal-real-time)) 
      (threads 
        (loop for t- from 0 below +NUM-THREADS+ collect 
          (let ((thread-id t-)) 
            (sb-thread:make-thread 
              (lambda () 
                (simulate +TOTAL-SIMULATIONS+ thread-id)))))) 
      (total-success-count 
        (reduce #'+ 
          (mapcar #'sb-thread:join-thread threads))) 
      (probability 
        (/ 
          (float total-success-count) +TOTAL-SIMULATIONS+))) 
    (format t "Probability: ~,9F~%" probability) 
    (let* 
      ( 
        (end-time 
          (get-internal-real-time)) 
        (elapsed-time 
          (/ 
            (float 
              (- end-time start-time)) internal-time-units-per-second))) 
      (format t "Execution Time: ~,3F s~%" elapsed-time)))) 
(main)