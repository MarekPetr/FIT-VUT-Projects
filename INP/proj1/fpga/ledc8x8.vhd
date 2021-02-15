library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.std_logic_arith.all;
use IEEE.std_logic_unsigned.all;

entity ledc8x8 is
	port (
		LED : out std_logic_vector(0 to 7);
		ROW : out std_logic_vector(7 downto 0);
		SMCLK : in std_logic;
		RESET : in std_logic
	);
end ledc8x8;


architecture main of ledc8x8 is
signal clock_count: std_logic_vector(7 downto 0);
signal clock_enable: std_logic;
signal leds: std_logic_vector(7 downto 0);
signal rows: std_logic_vector(7 downto 0);

begin

    clock_gen: process(RESET, SMCLK)
    begin
	if RESET = '1' then
		clock_count <= "00000000";
      elsif (SMCLK = '1') and (SMCLK'event) then
            clock_count <= clock_count + 1;
      end if;
    end process clock_gen;

    	clock_enable <= '1' when clock_count = "11111111" else '0';

	rows_rotation: process(RESET, SMCLK, clock_enable)
	begin	
	  if RESET = '1' then
			rows <= "10000000"; 
		elsif (clock_enable = '1') and (SMCLK'event) and (SMCLK = '1') then
			rows <= rows(0) & rows(7 downto 1);
		end if;
	end process rows_rotation;
	

	decoder: process(rows)
	begin
		case rows is
			when "00000001" => leds <= "00001111";
			when "00000010" => leds <= "01101111";
			when "00000100" => leds <= "00001111";
			when "00001000" => leds <= "01111111";
			when "00010000" => leds <= "01101110";
			when "00100000" => leds <= "11100100";
			when "01000000" => leds <= "11101010";
			when "10000000" => leds <= "11101110";
			
			when others     => leds <= "11111111";
		end case;
	end process decoder;


	ROW <= rows;
	LED <= leds;

end architecture main;