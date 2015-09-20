#!/usr/bin/env ruby

class NetlightMessage
  
  attr_accessor :operation, :unit, :red, :green, :blue, :sound_code, :timeout_seconds, :future_red, :future_green, :future_blue, :future_sound_code
  
  OPERATION_SET_UNIT  = 0
  
  def self.SOUNDS
    {
    :none => 0,
    :silence => 1,
    :up => 2,
    :down => 3,
    :short => 4,
    :long => 5,
    :chirp => 6,
    :rise => 7,
    :siren => 8,
    :xmas => 9
  }
  end
  
  def initialize
    @operation = OPERATION_SET_UNIT
    @unit = 0
    @red = 0
    @green = 0
    @blue = 0
    @sound_code = 0
    @timeout_seconds = 0
    @future_red = 0
    @future_green = 0
    @future_blue = 0
    @future_sound_code =0
  end
  
  def to_a
    [operation, unit, red, green, blue, sound_code, timeout_seconds, future_red, future_green, future_blue, future_sound_code]
  end
    
  
  def to_s
    to_a.pack('CCCCCCL<CCCC')
  end
  
  def sound( type, repeat = 0)
    @sound_code = NetlightMessage.SOUNDS[type] + ( repeat * 16 )
  end

  def future_sound( type, repeat = 0)
    @future_sound_code = NetlightMessage.SOUNDS[type] + ( repeat * 16 )
  end
  
end
