  ```sql

CREATE TABLE financial
    (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      name NVARCHAR(50) NOT NULL,
      description NTEXT DEFAULT NULL,
      rule BIT DEFAULT 0, -- DICD = 0, DDCI =1
      placeholder BIT DEFAULT 0
    );

CREATE INDEX idx_financial_id ON financial (id);

CREATE TABLE financial_path
    (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      ancestor INTEGER NOT NULL,
      descendant INTEGER NOT NULL,
      distance TINYINT NOT NULL CHECK (distance >= 0),
      FOREIGN KEY (ancestor) REFERENCES financial(id),
      FOREIGN KEY (descendant) REFERENCES financial(id),
      UNIQUE (ancestor, descendant)
    );

CREATE TABLE financial_addition
    (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      financial_id INTEGER NOT NULL,
      price REAL NOT NULL,
      FOREIGN KEY (financial_id) REFERENCES financial(id)
    );

CREATE TABLE financial_transaction
    (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      debit INTEGER DEFAULT NULL,
      credit INTEGER DEFAULT NULL,
      description NTEXT DEFAULT NULL,
      amount MONEY DEFAULT NULL CHECK (amount >=0),
      status BIT DEFAULT 0,
      document NTEXT DEFAULT NULL,
      post_date VARCHAR(10) DEFAULT NULL,
      FOREIGN KEY (debit) REFERENCES financial(id),
      FOREIGN KEY (credit) REFERENCES financial(id)
    );

CREATE INDEX idx_financial_transaction_id ON financial_transaction (id);

INSERT INTO financial (name, description, rule, placeholder) VALUES ('A', 'Node A', 0, 1);
INSERT INTO financial (name, description, rule, placeholder) VALUES ('B', 'Node B', 0, 0);
INSERT INTO financial (name, description, rule, placeholder) VALUES ('C', 'Node C', 0, 0);
INSERT INTO financial (name, description, rule, placeholder) VALUES ('D', 'Node D', 0, 0);
INSERT INTO financial (name, description, rule, placeholder) VALUES ('E', 'Node E', 0, 0);
INSERT INTO financial (name, description, rule, placeholder) VALUES ('F', 'Node F', 0, 0);
INSERT INTO financial (name, description, rule, placeholder) VALUES ('G', 'Node G', 0, 0);
INSERT INTO financial (name, description, rule, placeholder) VALUES ('H', 'Node H', 0, 0);
INSERT INTO financial (name, description, rule, placeholder) VALUES ('I', 'Node I', 0, 0);
INSERT INTO financial (name, description, rule, placeholder) VALUES ('J', 'Node J', 0, 0);
INSERT INTO financial (name, description, rule, placeholder) VALUES ('K', 'Node K', 0, 0);
INSERT INTO financial (name, description, rule, placeholder) VALUES ('L', 'Node L', 0, 0);
INSERT INTO financial (name, description, rule, placeholder) VALUES ('M', 'Node M', 0, 0);
INSERT INTO financial (name, description, rule, placeholder) VALUES ('N', 'Node N', 0, 0);

INSERT INTO financial_path (ancestor, descendant, distance) VALUES (14, 14, 0);
INSERT INTO financial_path (ancestor, descendant, distance) VALUES (1, 1, 0);
INSERT INTO financial_path (ancestor, descendant, distance) VALUES (2, 2, 0);
INSERT INTO financial_path (ancestor, descendant, distance) VALUES (3, 3, 0);
INSERT INTO financial_path (ancestor, descendant, distance) VALUES (4, 4, 0);
INSERT INTO financial_path (ancestor, descendant, distance) VALUES (5, 5, 0);
INSERT INTO financial_path (ancestor, descendant, distance) VALUES (6, 6, 0);
INSERT INTO financial_path (ancestor, descendant, distance) VALUES (7, 7, 0);
INSERT INTO financial_path (ancestor, descendant, distance) VALUES (8, 8, 0);
INSERT INTO financial_path (ancestor, descendant, distance) VALUES (9, 9, 0);
INSERT INTO financial_path (ancestor, descendant, distance) VALUES (10, 10, 0);
INSERT INTO financial_path (ancestor, descendant, distance) VALUES (11, 11, 0);
INSERT INTO financial_path (ancestor, descendant, distance) VALUES (12, 12, 0);
INSERT INTO financial_path (ancestor, descendant, distance) VALUES (13, 13, 0);
INSERT INTO financial_path (ancestor, descendant, distance) VALUES (1, 2, 1);
INSERT INTO financial_path (ancestor, descendant, distance) VALUES (1, 3, 1);
INSERT INTO financial_path (ancestor, descendant, distance) VALUES (1, 4, 1);
INSERT INTO financial_path (ancestor, descendant, distance) VALUES (1, 5, 1);
INSERT INTO financial_path (ancestor, descendant, distance) VALUES (1, 6, 1);
INSERT INTO financial_path (ancestor, descendant, distance) VALUES (1, 7, 1);
INSERT INTO financial_path (ancestor, descendant, distance) VALUES (1, 8, 1);
INSERT INTO financial_path (ancestor, descendant, distance) VALUES (1, 9, 1);
INSERT INTO financial_path (ancestor, descendant, distance) VALUES (1, 10, 1);
INSERT INTO financial_path (ancestor, descendant, distance) VALUES (1, 11, 1);
INSERT INTO financial_path (ancestor, descendant, distance) VALUES (1, 12, 1);

INSERT INTO financial_transaction (debit, credit, description, amount, post_date, document) VALUES (2,3,"",500.00,"2017-09-18","A;B;C");
INSERT INTO financial_transaction (debit, credit, description, amount, post_date, document) VALUES (2,3,"",700.00,"2018-08-17","B;C");
INSERT INTO financial_transaction (debit, credit, description, amount, post_date, document) VALUES (2,4,"",800.00,"2019-07-16","B;C;D");
INSERT INTO financial_transaction (debit, credit, description, amount, post_date, document) VALUES (2,5,"",900.00,"2011-06-15","A;C");
INSERT INTO financial_transaction (debit, credit, description, amount, post_date, document) VALUES (3,4,"",600.00,"2012-05-14","A;D");
INSERT INTO financial_transaction (debit, credit, description, amount, post_date, document) VALUES (7,2,"",700.00,"2013-04-13","B;E");
INSERT INTO financial_transaction (debit, credit, description, amount, post_date, document) VALUES (8,2,"",800.00,"2014-03-12","C;F");
INSERT INTO financial_transaction (debit, credit, description, amount, post_date, document) VALUES (9,2,"",900.00,"2015-02-11","C;D");
INSERT INTO financial_transaction (debit, credit, description, amount, post_date, document) VALUES (10,2,"",1000.00,"2016-01-10","G;F");
INSERT INTO financial_transaction (debit, credit, description, amount, post_date, document) VALUES (11,2,"",1100.00,"2022-11-09","H;G");
INSERT INTO financial_transaction (debit, credit, description, amount, post_date, document) VALUES (12,4,"",1200.00,"2023-12-08","E;N");
INSERT INTO financial_transaction (debit, credit, description, amount, post_date, document) VALUES (2,12,"",7000.00,"2024-10-07","W;B");
INSERT INTO financial_transaction (debit, credit, description, amount, post_date, document) VALUES (4, 2, 'Transaction 1', 500.00,"2017-10-18","B;N");
INSERT INTO financial_transaction (debit, credit, description, amount, post_date, document) VALUES (5, 3, 'Transaction 2', 700.00,"2018-09-17","B;T");
INSERT INTO financial_transaction (debit, credit, description, amount, post_date, document) VALUES (5, 3, 'Transaction 3', 300.00,"2019-08-16","C;F");
INSERT INTO financial_transaction (debit, credit, description, amount, post_date, document) VALUES (3, 4, 'Transaction 4', 900.00,"2011-07-15","I;D");
INSERT INTO financial_transaction (debit, credit, description, amount, post_date, document) VALUES (4, 5, 'Transaction 5', 800.00,"2012-06-14","N;B");
INSERT INTO financial_transaction (debit, credit, description, amount, post_date, document) VALUES (5, 6, 'Transaction 6', 600.00,"2013-05-13","N;V");
INSERT INTO financial_transaction (debit, credit, description, amount, post_date, document) VALUES (6, 7, 'Transaction 7', 750.00,"2010-04-12","H;W");
INSERT INTO financial_transaction (debit, credit, description, amount, post_date, document) VALUES (7, 8, 'Transaction 8', 250.00,"2007-03-11","X;Z");
INSERT INTO financial_transaction (debit, credit, description, amount, post_date, document) VALUES (8, 9, 'Transaction 9', 1200.00,"2008-02-10","Y;T;X");
INSERT INTO financial_transaction (debit, credit, description, amount, post_date, document) VALUES (9, 10, 'Transaction 10', 350.00,"2009-01-08","X;Y");

  ```
