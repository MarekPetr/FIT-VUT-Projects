DROP SEQUENCE id_inc;

DROP TABLE zlodej CASCADE CONSTRAINTS;
DROP TABLE zlocin CASCADE CONSTRAINTS;
DROP TABLE spachal CASCADE CONSTRAINTS;
DROP TABLE evidovan_v CASCADE CONSTRAINTS;
DROP TABLE vlastni_vybaneni CASCADE CONSTRAINTS;
DROP TABLE vybaveni_je_typu CASCADE CONSTRAINTS;
DROP TABLE skoleni_se_tyka CASCADE CONSTRAINTS;
DROP TABLE absolvoval CASCADE CONSTRAINTS;
DROP TABLE vlastni_povoleni CASCADE CONSTRAINTS;
DROP TABLE zlocin_je_typu CASCADE CONSTRAINTS;
DROP TABLE rajon CASCADE CONSTRAINTS;
DROP TABLE typ_zlocinu CASCADE CONSTRAINTS;
DROP TABLE bezny_zlodej CASCADE CONSTRAINTS;
DROP TABLE sef CASCADE CONSTRAINTS;

CREATE TABLE zlodej(
    prezdivka VARCHAR(15) PRIMARY KEY,
    realne_jmeno VARCHAR(30) NOT NULL,
    stav INT NOT NULL,
    vek INT NOT NULL,
    vypsana_odmena NUMBER NOT NULL
);

CREATE TABLE zlocin(
    ID_zlocinu INT PRIMARY KEY,
    korist NUMBER NOT NULL,
    spachan_v VARCHAR(30) NOT NULL
);

CREATE TABLE spachal(
    prezdivka VARCHAR(15),
    ID_zlocinu INT,
    PRIMARY KEY(prezdivka, ID_zlocinu)
);

CREATE TABLE evidovan_v(
    prezdivka VARCHAR(15) NOT NULL,
    poloha_rajonu VARCHAR(30) NOT NULL,
    PRIMARY KEY(prezdivka, poloha_rajonu)
);

CREATE TABLE vlastni_vybaneni(
    prezdivka VARCHAR(15),
    ID_vybaveni INT,
    od DATE,
    do DATE,
    PRIMARY KEY(prezdivka, ID_vybaveni, od)
);

CREATE TABLE vybaveni_je_typu(
    ID_vybaveni INT PRIMARY KEY,
    nazev_typu_vybaveni VARCHAR(20)
);

CREATE TABLE skoleni_se_tyka(
    ID_skoleni INT PRIMARY KEY, 
    datum_skoleni DATE NOT NULL,
    nazev_typu_zlocinu VARCHAR(20),
    nazev_typu_vybaveni VARCHAR(20)
);

CREATE TABLE absolvoval(
    prezdivka VARCHAR(15),
    ID_skoleni INT,
    PRIMARY KEY(prezdivka, ID_skoleni)
);

CREATE TABLE vlastni_povoleni(
    prezdivka VARCHAR(15),
    nazev_typu_zlocinu VARCHAR(20),
    PRIMARY KEY(prezdivka, nazev_typu_zlocinu)
);

CREATE TABLE zlocin_je_typu(
    id_zlocinu INT PRIMARY KEY,
    nazev_typu_zlocinu VARCHAR(20) NOT NULL
);

CREATE TABLE rajon(
    poloha_rajonu VARCHAR(30) PRIMARY KEY,
    pocet_lidi INT NOT NULL,
    kapacita_zlodeju INT NOT NULL,
    uloupena_korist NUMBER NOT NULL,
    dostupne_bohatsvi NUMBER NOT NULL
);

CREATE TABLE typ_zlocinu(
    nazev_typu_zlocinu VARCHAR(20) PRIMARY KEY,
    mira_obtiznosti_provedeni INT NOT NULL,    
    mira_obtiznosti_proskoleni INT NOT NULL,
    detaily VARCHAR(40)
);

CREATE TABLE bezny_zlodej(
    prezdivka VARCHAR(15) PRIMARY KEY,
    pocet_udelenych_povoleni INT NOT NULL
);

CREATE TABLE sef(
    prezdivka VARCHAR(15) PRIMARY KEY,
    pocet_vydanych_povoleni INT NOT NULL
);

-------------------------------------------------------------
------------------------ TRIGGRY ----------------------------
-------------------------------------------------------------
CREATE SEQUENCE id_inc	--autoincrement
MINVALUE 1
START WITH 1
INCREMENT BY 1
NOCACHE;

CREATE OR REPLACE TRIGGER Vygenerovani_id		--vygenerovani primarniho klice
BEFORE INSERT ON  zlocin
FOR EACH ROW
BEGIN:
	NEW.ID_zlocinu := id_inc.NEXTVAL;
END;
/

CREATE OR REPLACE TRIGGER Update_povoleni
AFTER INSERT ON vlastni_povoleni
FOR EACH ROW
BEGIN
    UPDATE bezny_zlodej
    SET pocet_udelenych_povoleni = pocet_udelenych_povoleni + 1
    WHERE :NEW.prezdivka = bezny_zlodej.prezdivka;  
END;
/

----------------------- CHECK --------------------------------

ALTER TABLE zlocin
ADD CONSTRAINT check_id_zlocinu_zlocin CHECK (id_zlocinu > 0);

ALTER TABLE zlocin_je_typu
ADD CONSTRAINT check_id_zlocinu_typ CHECK (id_zlocinu > 0);

ALTER TABLE skoleni_se_tyka
ADD CONSTRAINT check_id_skoleni_tyka_se CHECK (ID_skoleni > 0);

ALTER TABLE absolvoval
ADD CONSTRAINT check_id_skoleni_absolv CHECK (ID_skoleni > 0);

--------------------------------------------------------------
ALTER TABLE zlocin
ADD CONSTRAINT fk_spachan_v FOREIGN KEY (spachan_v)
REFERENCES rajon(poloha_rajonu);

ALTER TABLE spachal
ADD CONSTRAINT fk_spachal_kdo FOREIGN KEY (prezdivka)
REFERENCES zlodej(prezdivka) ON DELETE CASCADE;
ALTER TABLE spachal
ADD CONSTRAINT fk_spachal_co FOREIGN KEY (ID_zlocinu)
REFERENCES zlocin(ID_zlocinu) ON DELETE CASCADE;

ALTER TABLE evidovan_v
ADD CONSTRAINT fk_evidovan_kdo FOREIGN KEY (prezdivka)
REFERENCES zlodej(prezdivka) ON DELETE CASCADE;
ALTER TABLE evidovan_v
ADD CONSTRAINT fk_evidovan_v_kde FOREIGN KEY (poloha_rajonu)
REFERENCES rajon(poloha_rajonu) ON DELETE CASCADE;

ALTER TABLE vlastni_vybaneni
ADD CONSTRAINT fk_vlastni_vybaneni_kdo FOREIGN KEY (prezdivka)
REFERENCES zlodej(prezdivka) ON DELETE CASCADE;
ALTER TABLE vlastni_vybaneni
ADD CONSTRAINT fk_vlastni_vybaneni_co FOREIGN KEY (ID_vybaveni)
REFERENCES vybaveni_je_typu(ID_vybaveni) ON DELETE CASCADE;

ALTER TABLE skoleni_se_tyka
ADD CONSTRAINT fk_nazev_typu_zlocinu FOREIGN KEY (nazev_typu_zlocinu)
REFERENCES typ_zlocinu(nazev_typu_zlocinu);

ALTER TABLE absolvoval
ADD CONSTRAINT fk_absolvoval_kdo FOREIGN KEY (prezdivka)
REFERENCES zlodej(prezdivka) ON DELETE CASCADE;
ALTER TABLE absolvoval
ADD CONSTRAINT fk_absolvoval_co FOREIGN KEY (ID_skoleni)
REFERENCES skoleni_se_tyka(ID_skoleni) ON DELETE CASCADE;

ALTER TABLE vlastni_povoleni
ADD CONSTRAINT fk_vlastni_povoleni_kdo FOREIGN KEY (prezdivka)
REFERENCES zlodej(prezdivka) ON DELETE CASCADE;
ALTER TABLE vlastni_povoleni
ADD CONSTRAINT fk_vlastni_povoleni_co FOREIGN KEY (nazev_typu_zlocinu)
REFERENCES typ_zlocinu(nazev_typu_zlocinu) ON DELETE CASCADE;

ALTER TABLE zlocin_je_typu
ADD CONSTRAINT fk_zlocin_je_typu_kdo FOREIGN KEY (ID_zlocinu)
REFERENCES zlocin(ID_zlocinu) ON DELETE CASCADE;
ALTER TABLE zlocin_je_typu
ADD CONSTRAINT fk_zlocin_je_typu FOREIGN KEY (nazev_typu_zlocinu)
REFERENCES typ_zlocinu(nazev_typu_zlocinu) ON DELETE CASCADE;

GRANT ALL on zlodej to xmarek66;
GRANT ALL on zlocin to xmarek66;
GRANT ALL on spachal to xmarek66;
GRANT ALL on evidovan_v to xmarek66;
GRANT ALL on vlastni_vybaneni to xmarek66;
GRANT ALL on vybaveni_je_typu to xmarek66;
GRANT ALL on skoleni_se_tyka to xmarek66;
GRANT ALL on absolvoval to xmarek66;
GRANT ALL on vlastni_povoleni to xmarek66;
GRANT ALL on zlocin_je_typu to xmarek66;
GRANT ALL on rajon to xmarek66;
GRANT ALL on typ_zlocinu to xmarek66;
GRANT ALL on bezny_zlodej to xmarek66;
GRANT ALL on sef to xmarek66;

INSERT INTO zlodej
VALUES('Vendeta', 'Lubos Novak', 1, 25, 100);
INSERT INTO zlodej
VALUES('Mrchozrout', 'Lebeda Stary', 0, 55, 10000);
INSERT INTO zlodej
VALUES('Sakrast', 'Jan Kroutitel', 1, 70, 5555);
INSERT INTO zlodej
VALUES('Teror', 'Bohous Maly', 0, 35, 0);
INSERT INTO zlodej
VALUES('Micik', 'Patrik Dlouhy', 1, 35, 135000);

INSERT INTO rajon
VALUES('Benatky', 10000, 100, 1000, 1000000);
INSERT INTO rajon
VALUES('Rim', 200000, 2000, 200000, 2000000);
INSERT INTO rajon
VALUES('Florencie', 30000, 300, 3000, 300000);
INSERT INTO rajon
VALUES('Tisnov', 40000, 400, 4000, 4000000);
INSERT INTO rajon
VALUES('Wellington', 50000, 500, 5000, 5000000);

INSERT INTO zlocin
VALUES(1, 50, 'Benatky');
INSERT INTO zlocin
VALUES(2, 5000, 'Rim');
INSERT INTO zlocin
VALUES(3, 2500, 'Florencie');
INSERT INTO zlocin
VALUES(4, 300000, 'Wellington');

INSERT INTO vybaveni_je_typu
VALUES(1, 'Remdich');
INSERT INTO vybaveni_je_typu
VALUES(2, 'Pacidlo');
INSERT INTO vybaveni_je_typu
VALUES(3, 'Baseballova palka');
INSERT INTO vybaveni_je_typu
VALUES(4, 'Maceta');

INSERT INTO bezny_zlodej
VALUES('Vendeta', 0);
INSERT INTO bezny_zlodej
VALUES('Mrchozrout', 0);
INSERT INTO bezny_zlodej
VALUES('Teror', 0);

INSERT INTO sef
VALUES('Sakrast', 4);
INSERT INTO sef
VALUES('Micik', 5);

INSERT INTO typ_zlocinu
VALUES('vyloupeni banky', 9, 9, 'deset let ve vezeni');
INSERT INTO typ_zlocinu
VALUES('pasovani', 7, 5, 'Tezky zlocin');
INSERT INTO typ_zlocinu
VALUES('vloupani', 5, 3, 'Na to je potreba znat techniky');
INSERT INTO typ_zlocinu
VALUES('unos', 7, 6, 'Dobre je soustredit se na motiv');
INSERT INTO typ_zlocinu
VALUES('vrazda', 8, 6, 'Nejtvrdsi mozny trest');

INSERT INTO zlocin_je_typu
VALUES(1, 'vyloupeni banky');
INSERT INTO zlocin_je_typu
VALUES(2, 'pasovani');
INSERT INTO zlocin_je_typu
VALUES(3, 'vloupani');
INSERT INTO zlocin_je_typu
VALUES(4, 'unos');

INSERT INTO spachal
VALUES('Vendeta', 1);
INSERT INTO spachal
VALUES('Sakrast', 2);
INSERT INTO spachal
VALUES('Mrchozrout', 3);
INSERT INTO spachal
VALUES('Micik', 1);
INSERT INTO spachal
VALUES('Micik', 4);

INSERT INTO evidovan_v
VALUES('Vendeta', 'Benatky');
INSERT INTO evidovan_v
VALUES('Vendeta', 'Rim');
INSERT INTO evidovan_v
VALUES('Sakrast', 'Rim');
INSERT INTO evidovan_v
VALUES('Mrchozrout', 'Florencie');
INSERT INTO evidovan_v
VALUES('Teror', 'Tisnov');
INSERT INTO evidovan_v
VALUES('Micik', 'Wellington');

INSERT INTO vlastni_vybaneni
VALUES('Vendeta', 1, TO_DATE('2018-02-13', 'YYYY-MM-DD'), TO_DATE('2018-02-15', 'YYYY-MM-DD'));
INSERT INTO vlastni_vybaneni
VALUES('Sakrast', 2, TO_DATE('2010-01-01', 'YYYY-MM-DD'), TO_DATE('2011-02-11', 'YYYY-MM-DD'));
INSERT INTO vlastni_vybaneni
VALUES('Mrchozrout', 3, TO_DATE('2008-02-15', 'YYYY-MM-DD'), TO_DATE('2010-03-02', 'YYYY-MM-DD'));
INSERT INTO vlastni_vybaneni
VALUES('Teror', 4, TO_DATE('2007-06-15', 'YYYY-MM-DD'), TO_DATE('2007-06-20', 'YYYY-MM-DD'));
INSERT INTO vlastni_vybaneni
VALUES('Micik', 4, TO_DATE('2007-05-20', 'YYYY-MM-DD'), TO_DATE('2007-05-21', 'YYYY-MM-DD'));

INSERT INTO skoleni_se_tyka
VALUES(1, TO_DATE('2018-02-10', 'YYYY-MM-DD'),NULL, 'Pacidlo');
INSERT INTO skoleni_se_tyka
VALUES(2, TO_DATE('2007-06-02', 'YYYY-MM-DD'), NULL, 'Maceta');
INSERT INTO skoleni_se_tyka
VALUES(3, TO_DATE('2018-02-10', 'YYYY-MM-DD'), 'pasovani', NULL );
INSERT INTO skoleni_se_tyka
VALUES(4, TO_DATE('2008-10-17', 'YYYY-MM-DD'), 'unos', NULL);
INSERT INTO skoleni_se_tyka
VALUES(5, TO_DATE('2007-04-14', 'YYYY-MM-DD'), 'vloupani', NULL);

INSERT INTO absolvoval
VALUES('Vendeta', 1);
INSERT INTO absolvoval
VALUES('Teror', 2);
INSERT INTO absolvoval
VALUES('Micik', 2);
INSERT INTO absolvoval
VALUES('Vendeta', 4);
INSERT INTO absolvoval
VALUES('Sakrast', 3);

INSERT INTO vlastni_povoleni
VALUES('Teror', 'vyloupeni banky');
INSERT INTO vlastni_povoleni
VALUES('Mrchozrout', 'unos');
INSERT INTO vlastni_povoleni
VALUES('Mrchozrout', 'vrazda');
INSERT INTO vlastni_povoleni
VALUES('Vendeta', 'pasovani');
INSERT INTO vlastni_povoleni
VALUES('Vendeta', 'unos');

--PART 2
---------------------------------------------------------------------------------------------------
-- JOIN 2 tabulky - zlodej, spachal
-- Vypise ktery zlodej spachal ktere zlociny.
-- Zlodeje, kteri nespachali zadne preskoci.
SELECT A.prezdivka, A.realne_jmeno, B.ID_zlocinu
FROM zlodej A
LEFT JOIN spachal B ON A.prezdivka = B.prezdivka
WHERE B.ID_zlocinu is NOT NULL;

-- JOIN 2 tabulky - sef, absolvoval
-- Vypise ktery sef absolvoval ktera skoleni.
SELECT A.prezdivka, B.ID_skoleni
FROM sef A
LEFT JOIN absolvoval B ON A.prezdivka = B.prezdivka;

-- JOIN 3 tabulky - C zlodej, A spachal, B zlocin
-- Vypise ve kterem rajonu spachal jaky zlodej jaky zlocin 
-- -> vypsana prezdivka, realne jmeno, poloha_rajonu
SELECT C.prezdivka, C.realne_jmeno, B.ID_zlocinu, B.spachan_v
FROM spachal A
LEFT JOIN zlocin B ON A.ID_zlocinu = B.ID_zlocinu
LEFT JOIN zlodej C ON A.prezdivka = C.prezdivka;

--GROUP BY - spachal, zlocin
-- Vypise kolik dohromady kazdy zlodej ukoristil, serazeny podle jmeni
SELECT A.prezdivka, SUM(korist) AS jmeni
FROM zlocin B
LEFT JOIN spachal A ON A.ID_zlocinu = B.ID_zlocinu
GROUP BY A.prezdivka
ORDER BY jmeni DESC;

--GROUP BY - 
-- Vypise celkovou vypsanou odmenu na vsechny zive zlodeje
SELECT SUM(vypsana_odmena) AS vypsana_odmena_zivi
FROM zlodej
WHERE (stav = 1)
GROUP BY stav
ORDER BY vypsana_odmena_zivi DESC;

--EXISTS
-- Vypise vsechny zlodeje, kteri spachali nejaky zlocin
SELECT prezdivka AS zlocinci
FROM spachal
WHERE EXISTS (SELECT ID_zlocinu FROM spachal)
GROUP BY prezdivka
ORDER BY prezdivka;

-- IN (vnoreny select) - absolvoval, skoleni_se_tyka
-- Vypis zlodeju, kteri byli dne 2.6.2007 na skoleni
SELECT A.prezdivka
FROM absolvoval A
WHERE A.ID_skoleni
    IN(SELECT B.ID_skoleni FROM skoleni_se_tyka B WHERE B.datum_skoleni = date '2007-06-02');

--PART 3
-------------------------------------------------------------------------------------------------
---UKAZKA TRIGGRU---
SELECT * FROM zlocin;

SELECT B.prezdivka, B.pocet_udelenych_povoleni AS pred_udelenim_povoleni
FROM bezny_zlodej B
WHERE B.prezdivka = 'Vendeta';

INSERT INTO vlastni_povoleni
VALUES('Vendeta', 'vyloupeni banky');

SELECT B.prezdivka, B.pocet_udelenych_povoleni AS po_udeleni_povoleni
FROM bezny_zlodej B
WHERE B.prezdivka = 'Vendeta';
--------------------

--------------------
--------------------
-----PROCEDURES-----
-- Vypise, zda existuje zlocin se zadanym ID a pokud ano, vypise jakeho je typu a detaily tohoto zlocinu
SET serveroutput ON;
CREATE OR REPLACE PROCEDURE detail_zlocin(id_z zlocin_je_typu.ID_zlocinu%TYPE)
IS
    typ zlocin_je_typu.nazev_typu_zlocinu%TYPE;
    det typ_zlocinu%ROWTYPE;
BEGIN
    SELECT nazev_typu_zlocinu INTO typ FROM zlocin_je_typu WHERE zlocin_je_typu.id_zlocinu = id_z;
    SELECT * INTO det FROM typ_zlocinu WHERE typ_zlocinu.nazev_typu_zlocinu = typ;
    dbms_output.put_line('Zlocin s ID ' || id_z || ' je typu ' || typ || ', jeho mira obtiznosti provedeni: ' || det.mira_obtiznosti_provedeni || ', mira obtiznosit proskolni: ' || det.mira_obtiznosti_proskoleni || ' a detaily: ' || det.detaily);
EXCEPTION
    WHEN NO_DATA_FOUND THEN
        dbms_output.put_line('Zlocin se zadanym ID neexistuje');
END;
/

exec detail_zlocin(1);

-- Vypise zlodeje, kteri maji rovnaky (nebo mensi nebo vetsi) pocet udelenych povoleni
SET serveroutput ON;
CREATE OR REPLACE PROCEDURE pocty_povoleni(prezdivka_z bezny_zlodej.prezdivka%TYPE)
IS
    CURSOR b_zlodej IS SELECT * FROM bezny_zlodej;
    info_zlodej b_zlodej%ROWTYPE;
    pocet_povoleni bezny_zlodej.pocet_udelenych_povoleni%TYPE;
    prez bezny_zlodej.prezdivka%TYPE;
BEGIN
    SELECT pocet_udelenych_povoleni INTO pocet_povoleni FROM bezny_zlodej WHERE prezdivka_z = bezny_zlodej.prezdivka;
    SELECT prezdivka INTO prez FROM bezny_zlodej WHERE prezdivka_z = bezny_zlodej.prezdivka;
    OPEN b_zlodej;
    LOOP
        FETCH b_zlodej INTO info_zlodej;
        EXIT WHEN b_zlodej%NOTFOUND;
        IF (info_zlodej.prezdivka = prez)
            THEN
            dbms_output.put_line('Zlodej k porovnani poctu udelenych povoleni: ' || prez);
            ELSIF (info_zlodej.pocet_udelenych_povoleni = pocet_povoleni)
                THEN
                dbms_output.put_line('Zlodej ' || info_zlodej.prezdivka || ' ma stejny pocet udelenych povoleni jako ' || prez || '.');
            ELSIF (info_zlodej.pocet_udelenych_povoleni > pocet_povoleni)
                THEN
                dbms_output.put_line('Zlodej ' || info_zlodej.prezdivka || ' ma vetsi pocet udelenych povoleni nez ' || prez || '.');
            ELSIF (info_zlodej.pocet_udelenych_povoleni < pocet_povoleni)
                THEN
                dbms_output.put_line('Zlodej ' || info_zlodej.prezdivka || ' ma mensi pocet udelenych povoleni nez ' || prez || '.');
            END IF;
    END LOOP;
EXCEPTION
    WHEN NO_DATA_FOUND
        THEN
        dbms_output.put_line('Zlodej se zadanou prezdivkou neexistuje.');
END;
/
exec pocty_povoleni('Vendeta');

-------------------------------------------------------------
-----------------EXPLAIN PLAN - vytvoreni indexu-------------
-------------------------------------------------------------
EXPLAIN PLAN FOR
SELECT A.prezdivka, SUM(korist) AS jmeni
FROM zlocin B
LEFT JOIN spachal A ON A.ID_zlocinu = B.ID_zlocinu
WHERE (korist > 1000)
GROUP BY A.prezdivka
ORDER BY jmeni DESC;
SELECT * FROM TABLE(DBMS_XPLAN.display);

CREATE INDEX korist_index ON zlocin(korist);

EXPLAIN PLAN FOR
SELECT A.prezdivka, SUM(korist) AS jmeni
FROM zlocin B
LEFT JOIN spachal A ON A.ID_zlocinu = B.ID_zlocinu
WHERE (korist > 1000)
GROUP BY A.prezdivka
ORDER BY jmeni DESC;
SELECT * FROM TABLE(DBMS_XPLAN.display);
----------------------------------

-------------------------------------------------------------
---------------- MATERIALIZED VIEW --------------------------
-------------------------------------------------------------
-- zobrazeni dnu skoleni a jejich pocet v dany den
DROP MATERIALIZED VIEW skoleni_za_den;

CREATE MATERIALIZED VIEW LOG ON skoleni_se_tyka WITH PRIMARY KEY,ROWID(datum_skoleni) INCLUDING NEW VALUES;

CREATE MATERIALIZED VIEW skoleni_za_den
CACHE 
BUILD IMMEDIATE 
REFRESH FAST 
ENABLE QUERY REWRITE AS 
SELECT datum_skoleni as datum, COUNT(ID_skoleni) pocet_skoleni_za_den
FROM skoleni_se_tyka
GROUP BY datum_skoleni;

GRANT ALL ON skoleni_za_den TO xmarek66;

SELECT * FROM skoleni_za_den;

INSERT INTO skoleni_se_tyka(ID_skoleni,datum_skoleni)
VALUES(10,date '2050-01-03');
COMMIT;
SELECT * FROM skoleni_za_den;