USE tank;

CREATE TABLE `readings` (
	`timestamp` TIMESTAMP,
	`value`INT,
	PRIMARY KEY (`timestamp`)
) ENGINE=MyISAM;

GRANT ALL PRIVILEGES ON *.* TO 'root'@'%';


