package org.quickfix.field; 
import org.quickfix.BooleanField; 
import java.util.Date; 

public class TradedFlatSwitch extends BooleanField 
{ 

  public TradedFlatSwitch() 
  { 
    super(258);
  } 
  public TradedFlatSwitch(boolean data) 
  { 
    super(258, data);
  } 
} 