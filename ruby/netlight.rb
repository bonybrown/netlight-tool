#!/usr/bin/env ruby
require_relative 'netlight_message'
require_relative 'netlight_comms'
require 'optparse'

Options = Struct.new(:unit,:color,:sound,:repeat,:timeout,:future_color,:future_sound,:future_repeat,:device_address,:key_file)

class Parser
  def self.parse(options)
    args = Options.new(nil,nil,nil,0,0,nil,nil,0,nil,nil)

    opt_parser = OptionParser.new do |opts|
      opts.banner = "Usage: netlight.rb --unit N --color RRGGBB [other options] dns_name_or_address"

      opts.on("-uUNIT", "--unit=UNIT", /[0-4]/, "unit number; first unit is 0") do |n|
        args.unit = n.to_i
      end

      opts.on("-cRRGGBB", "--color=RRGGBB",/[0-f]{6}/, "the hex color code to set on the light unit") do |c|
        args.color = c
      end
      
      opts.on("-sSOUND", "--sound=SOUND", NetlightMessage.SOUNDS.keys, NetlightMessage.SOUNDS.keys.join('|') ) do |s|
        args.sound = s
      end

      opts.on("-rCOUNT", "--sound-repeat=COUNT", Integer, "number of times to repeat the sound") do |r|
        args.repeat = r.to_i
      end

      opts.on("-TSECONDS", "--future-timeout=SECONDS", Integer, "number of seconds until the future. Required to specify any of the following") do |t|
        args.timeout = t.to_i
      end

      opts.on("-CRRGGBB", "--future-color=RRGGBB",/[0-f]{6}/, "the hex color code to set on the light unit when the timeout occurs") do |c|
        args.future_color = c
      end

      opts.on("-SSOUND", "--future-sound=SOUND", NetlightMessage.SOUNDS.keys, NetlightMessage.SOUNDS.keys.join('|') ) do |s|
        args.future_sound = s
      end

      opts.on("-RCOUNT", "--future-sound-repeat=COUNT", Integer, "number of times to repeat the sound") do |r|
        args.future_repeat = r.to_i
      end
      
      opts.on("-kKEYFILE", "--key-file=KEYFILE", "filename containing the device's binary key" ) do |k|
        args.key_file = k
      end

      
      opts.on("-h", "--help", "Prints this help") do
        puts opts
        exit
      end
      
    end

    opt_parser.parse!(options)
    return args
  end
end
options = Parser.parse ARGV

options.device_address = ARGV[0]

if options.unit.nil? || options.color.nil? || options.device_address.nil?
  $stderr.puts("ERROR: unit, color and device address are required options")
  Parser.parse %w[--help]
end

if options.timeout > 0 && options.future_color.nil?
  $stderr.puts("ERROR: future-color is required when timeout specified")
  Parser.parse %w[--help]
end

if options.timeout == 0 && ( 
  !options.future_color.nil? || !options.future_sound.nil? || options.future_repeat != 0 )
  $stderr.puts("ERROR: timeout is required when any future action is specified")
  Parser.parse %w[--help]
end


message = NetlightMessage.new
message.unit = options.unit
message.red,message.green,message.blue = [options.color].pack('H*').unpack('CCC')
message.sound(options.sound, options.repeat) unless options.sound.nil?
message.timeout_seconds = options.timeout
message.future_red,message.future_green,message.future_blue = [options.future_color].pack('H*').unpack('CCC') unless options.future_color.nil?
message.future_sound(options.future_sound, options.future_repeat) unless options.future_sound.nil?

comms = NetlightComms.new( options.device_address )
puts "Found device: #{comms.device_id}, Protocol version #{comms.version_major}.#{comms.version_minor}"
comms.key_file = options.key_file unless options.key_file.nil?

comms.send_message( message )

comms.close