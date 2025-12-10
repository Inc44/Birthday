-- mysql -u root -p < birthday.sql
CREATE DATABASE IF NOT EXISTS simulate;
USE simulate;
SET
	@total_simulations = 1000000;
DELIMITER $$
CREATE
OR REPLACE PROCEDURE main(IN p_total_simulations INT)
BEGIN
DECLARE
v_days_in_year SMALLINT UNSIGNED DEFAULT 365;
DECLARE
v_people TINYINT UNSIGNED DEFAULT 24;
DECLARE
v_multiplier INT UNSIGNED DEFAULT 1664525;
DECLARE
v_increment INT UNSIGNED DEFAULT 1013904223;
DECLARE
v_uint32_t BIGINT UNSIGNED DEFAULT 4294967296;
DECLARE
v_state INT UNSIGNED DEFAULT NOW(6) MOD v_uint32_t;
DECLARE
v_sim INT UNSIGNED DEFAULT 0;
DECLARE
v_people_i TINYINT UNSIGNED;
DECLARE
v_birthday SMALLINT UNSIGNED;
DECLARE
v_exactly_two_count TINYINT UNSIGNED DEFAULT 0;
DECLARE
v_total_success_count INT UNSIGNED DEFAULT 0;
DECLARE
v_start_time DATETIME(6) DEFAULT NOW(6);
DECLARE
probability DOUBLE;
DECLARE
v_end_time DATETIME(6);
DECLARE
elapsed_time DOUBLE;
CREATE
OR REPLACE TEMPORARY TABLE birthdays (
	day SMALLINT PRIMARY KEY,
	count TINYINT DEFAULT 0
) ENGINE = MEMORY;
WHILE v_sim < p_total_simulations DO TRUNCATE TABLE birthdays;
SET
	v_people_i = 0;
WHILE v_people_i < v_people DO
SET
	v_state = (v_state * v_multiplier + v_increment) MOD v_uint32_t;
SET
	v_birthday = v_state MOD v_days_in_year;
INSERT INTO
	birthdays (day, count)
VALUES
	(v_birthday, 1) ON DUPLICATE KEY
UPDATE
	count = count + 1;
SET
	v_people_i = v_people_i + 1;
END WHILE;
SELECT
	COUNT(*) INTO v_exactly_two_count
FROM
	birthdays
WHERE
	count = 2;
SET
	v_total_success_count = v_total_success_count + (v_exactly_two_count = 1);
SET
	v_sim = v_sim + 1;
END WHILE;
SET
	probability = v_total_success_count / p_total_simulations;
SET
	v_end_time = NOW(6);
SET
	elapsed_time = TIMESTAMPDIFF(MICROSECOND, v_start_time, v_end_time) / 1000000;
SELECT
	CONCAT(
		'Probability: ',
		FORMAT(probability, 9)
	)
UNION
ALL
SELECT
	CONCAT(
		'Execution Time: ',
		FORMAT(elapsed_time, 3),
		' s'
	);
END
$$
DELIMITER ;
CALL main(@total_simulations);