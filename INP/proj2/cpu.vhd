-- cpu.vhd: Simple 8-bit CPU (BrainLove interpreter)
-- Copyright (C) 2017 Brno University of Technology,
--                    Faculty of Information Technology
-- Author(s): xmarek66
--

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;
use ieee.std_logic_unsigned.all;

-- ----------------------------------------------------------------------------
--                        Entity declaration
-- ----------------------------------------------------------------------------
entity cpu is
 port (
   CLK   : in std_logic;  -- hodinovy signal
   RESET : in std_logic;  -- asynchronni reset procesoru
   EN    : in std_logic;  -- povoleni cinnosti procesoru
 
   -- synchronni pamet ROM
   CODE_ADDR : out std_logic_vector(11 downto 0); -- adresa do pameti
   CODE_DATA : in std_logic_vector(7 downto 0);   -- CODE_DATA <- rom[CODE_ADDR] pokud CODE_EN='1'
   CODE_EN   : out std_logic;                     -- povoleni cinnosti
   
   -- synchronni pamet RAM
   DATA_ADDR  : out std_logic_vector(9 downto 0); -- adresa do pameti
   DATA_WDATA : out std_logic_vector(7 downto 0); -- mem[DATA_ADDR] <- DATA_WDATA pokud DATA_EN='1'
   DATA_RDATA : in std_logic_vector(7 downto 0);  -- DATA_RDATA <- ram[DATA_ADDR] pokud DATA_EN='1'
   DATA_RDWR  : out std_logic;                    -- cteni z pameti (DATA_RDWR='0') / zapis do pameti (DATA_RDWR='1')
   DATA_EN    : out std_logic;                    -- povoleni cinnosti
   
   -- vstupni port
   IN_DATA   : in std_logic_vector(7 downto 0);   -- IN_DATA obsahuje stisknuty znak klavesnice pokud IN_VLD='1' a IN_REQ='1'
   IN_VLD    : in std_logic;                      -- data platna pokud IN_VLD='1'
   IN_REQ    : out std_logic;                     -- pozadavek na vstup data
   
   -- vystupni port
   OUT_DATA : out  std_logic_vector(7 downto 0);  -- zapisovana data
   OUT_BUSY : in std_logic;                       -- pokud OUT_BUSY='1', LCD je zaneprazdnen, nelze zapisovat,  OUT_WE musi byt '0'
   OUT_WE   : out std_logic                       -- LCD <- OUT_DATA pokud OUT_WE='1' a OUT_BUSY='0'
 );
end cpu;


-- ----------------------------------------------------------------------------
--                      Architecture declaration
-- ----------------------------------------------------------------------------
architecture behavioral of cpu is

type fsm_state is 
(fetch, decode, 
inc_ptr, dec_ptr,
inc_data_1, inc_data_2,
dec_data_1, dec_data_2,
putchar_1, putchar_2,
getchar_1, getchar_2,
while_s_1, while_s_2,
while_e_1, while_e_2,
break_1,   break_2,
skip, halt);
signal nstate : fsm_state;
signal pstate : fsm_state;

signal pc_data : std_logic_vector(11 downto 0);
signal pc_inc : std_logic;
signal pc_dec : std_logic;

signal ptr_data : std_logic_vector(9 downto 0);
signal ptr_inc : std_logic;
signal ptr_dec : std_logic;

signal cnt_data : std_logic_vector(7 downto 0);
signal cnt_inc : std_logic;
signal cnt_dec : std_logic;
                        
signal sel : std_logic_vector(1 downto 0);

signal cnt_data_is_zero : std_logic;
signal rdata_is_zero : std_logic;
signal rdata_inc : std_logic_vector(7 downto 0);
signal rdata_dec : std_logic_vector(7 downto 0);

begin
-- Program counter PC
pc_cntr: process(CLK, RESET)
  begin
    if (RESET = '1') then
      pc_data <= (others => '0');
    elsif (CLK'event) and (CLK = '1') then
      if (pc_inc='1') then
        pc_data <= pc_data + 1;
      elsif (pc_dec='1') then
        pc_data <= pc_data - 1;
      end if;
    end if;
  end process;

CODE_ADDR <= pc_data;

-- Adress of a given cell in RAM
ptr_cntr: process(CLK, RESET)
  begin
    if (RESET = '1') then
      ptr_data <= (others => '0');
    elsif (CLK'event) and (CLK = '1') then
      if (ptr_inc='1') then
        ptr_data <= ptr_data + 1;
      elsif (ptr_dec='1') then
        ptr_data <= ptr_data - 1;
      end if;
    end if;
  end process;

DATA_ADDR <= ptr_data;

-- Adress of program in ROM
cnt_cntr: process(CLK, RESET)
  begin
    if (RESET = '1') then
      cnt_data <= (others => '0');
    elsif (CLK'event) and (CLK = '1') then
      if (cnt_inc='1') then
        cnt_data <= cnt_data + 1;
      elsif (cnt_dec='1') then
        cnt_data <= cnt_data - 1;
      end if;
    end if;
  end process;



-- Multiplexor to inc/dec values in DATA_RDATA
with sel select
	DATA_WDATA <= IN_DATA when "00",
	rdata_inc when "10",
	rdata_dec when "01",
	(others => '0') when others;

rdata_is_zero <= '1' when (DATA_RDATA = "00000000") else '0';
rdata_inc <= DATA_RDATA+"00000001";
rdata_dec <= DATA_RDATA-"00000001" ;

cnt_data_is_zero <= '1' when (cnt_data = "00000000") else '0';


-- FSM present state
fsm_pstate: process(CLK, RESET)
  begin
    if (RESET = '1') then
      pstate <= fetch;
    elsif (CLK'event) and (CLK = '1') then
      if(EN = '1') then
        pstate <= nstate;
      end if;
    end if;
  end process;

-- FSM next state logic, output logic
fsm_nstate: process (pstate, cnt_data_is_zero, rdata_is_zero, CLK, RESET, EN, CODE_DATA, DATA_RDATA, IN_DATA, IN_VLD, OUT_BUSY)
  begin                                                                                                                               
    nstate <= fetch;
    CODE_EN <= '0';
    DATA_EN <= '0';
    IN_REQ <= '0';
    OUT_WE <= '0';
    ptr_inc <= '0';
    ptr_dec <= '0';
    pc_inc <= '0';
    pc_dec <= '0';
    cnt_inc <= '0';
    cnt_dec <= '0';
    sel <= "00";

    case pstate is
      when fetch =>
        CODE_EN <= '1';
        nstate <= decode;
    
      when decode =>
        case CODE_DATA is
	    when X"2B" =>
            nstate <= inc_data_1;
          when X"2D" =>
            nstate <= dec_data_1;
          when X"3E" =>
            nstate <= inc_ptr;
          when X"3C" =>
            nstate <= dec_ptr;          
          when X"5B" =>
            nstate <= while_s_1;
          when X"5D" =>
            nstate <= while_e_1;
	    when X"7E" =>
	      nstate <= break_1;
	    when X"2C" =>
            nstate <= getchar_1;
          when X"2E" =>
            nstate <= putchar_1;          
          when X"00" =>
            nstate <= halt;
          when others =>
            nstate <= skip;
        end case;       
		    
      when inc_data_1 =>
	  DATA_EN   <= '1';
        DATA_RDWR <= '0';        
        nstate    <= inc_data_2;
    
      when inc_data_2 =>
        DATA_EN   <= '1';		
        DATA_RDWR <= '1';        
        pc_inc    <= '1';
        pc_dec    <= '0';
        sel       <= "10";
        nstate    <= fetch;
      
      when dec_data_1 =>
	  DATA_EN   <= '1';
        DATA_RDWR <= '0';        
        nstate    <= dec_data_2;
      
      when dec_data_2 =>        
        DATA_RDWR <= '1';
        DATA_EN   <= '1';
        pc_inc    <= '1';
	  pc_dec    <= '0';
	  sel       <= "01";
        nstate    <= fetch; 
		  
      when inc_ptr =>
        ptr_inc <= '1';
	  ptr_dec <= '0';
        pc_inc  <= '1';
	  pc_dec  <= '0';
        nstate  <= fetch;
      
      when dec_ptr =>
        ptr_dec <= '1';
	  ptr_inc <= '0';
        pc_inc  <= '1';
	  pc_dec  <= '0';
        nstate  <= fetch;
    	
	when getchar_1 =>	  
	  IN_REQ <= '1';
	  nstate <= getchar_2;

      when getchar_2 =>
        if (IN_VLD = '1') then
	    IN_REQ <= '0';
          DATA_RDWR <= '1';
          DATA_EN <= '1';
          pc_inc <= '1';
	    pc_dec <= '0';
	    sel <= "00";
          nstate <= fetch;  
        else
          nstate <= getchar_2;
        end if;
		  
	when putchar_1 =>
	  DATA_EN <= '1';
	  DATA_RDWR <= '0';					
	  nstate <= putchar_2;	

      when putchar_2 =>
        if(OUT_BUSY = '0') then
	    OUT_DATA <= DATA_RDATA; 
          OUT_WE <= '1'; 
	    pc_inc <= '1';
	    pc_dec <= '0';			          
          nstate <= fetch;          
        else
          nstate <= putchar_2;
        end if;	

      when while_s_1 =>
	  pc_inc <= '1';
	  pc_dec <= '0';
	  if (rdata_is_zero = '1') then
	    CODE_EN <= '1';
	    nstate <= while_s_2;
	  else
	    nstate <= fetch;
	  end if;
			 
	when while_s_2 =>
	  if (CODE_DATA = X"5D") then
	    nstate <= fetch;
	  else
	    nstate <= while_s_1;
	  end if;				
		    
	when while_e_1 =>
        if (rdata_is_zero = '1') then
          pc_inc <= '1';
	    pc_dec <= '0';
          nstate <= fetch;
        else
          CODE_EN <= '1';
          nstate <= while_e_2;
        end if;
      
      when while_e_2 =>
        if (CODE_DATA = X"5B") then
          nstate <= fetch;
        else
          pc_dec <= '1';
	    pc_inc <= '0';
          nstate <= while_e_1;
	  end if;
		  
      when break_1 =>
	  CODE_EN <= '1';
        pc_inc <= '1';
        pc_dec <= '0';
        nstate <= break_2;

      when break_2 =>
        if(CODE_DATA = X"5D") then
          nstate <= fetch;
        else
          CODE_EN <= '1';
          pc_inc <= '1';
	    pc_dec <= '0';
          nstate <= break_2;
        end if;		  
		
      when skip =>
        pc_inc <= '1';
	  pc_dec <= '0';
        nstate <= fetch;
      
      when halt =>
        nstate <= halt;
    
      when others => null;

    end case;
  end process;
end behavioral;