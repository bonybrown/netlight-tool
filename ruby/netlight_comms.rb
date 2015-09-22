#!/usr/bin/env ruby

require 'openssl'
require 'socket'

class NetlightComms
  
  attr_writer :key
  
  attr_reader :device_id, :version_major, :version_minor
  
  def initialize( address )
    @socket = TCPSocket.new address, 3601
    @device_id = @socket.gets("\0")
    @version_major,@version_minor= @socket.read(2).unpack('CC')
    @iv = @socket.read(16)
  end
  
  #Alternatively, you can call key= to set as a binary string directly
  def key_file=( filename )
    open(filename,'r') do |f|
      @key=f.read(16)
    end
  end
  
  def close
    @socket.close
  end
  
  def comms_encrypted?
    @version_major >= 2
  end
  
  def send_message( message )
    message = message.to_s if message.respond_to? :to_s
    message = encrypt_message( message ) if comms_encrypted?
    header = [0x17,@version_major,@version_minor, message.length]
    puts message.unpack('H*')
    @socket.write(header.pack('CCCS<') + message)
  end  
  
  def master_key
    raise ArgumentError.new('Key is required') if @key.nil?
    @master_key ||= OpenSSL::Digest::SHA1.digest(@key+@iv)[0..15]
  end
  
  def encrypt_message( message )
    sender_iv = OpenSSL::Random.random_bytes(16)
    message_hash = OpenSSL::Digest::SHA1.digest(message)
    cipher = OpenSSL::Cipher.new('AES-128-CBC')
    cipher.encrypt
    cipher.key = master_key
    cipher.iv = sender_iv
    encrypted_block = cipher.update(message+message_hash) + cipher.final
    sender_iv + encrypted_block
  end
end