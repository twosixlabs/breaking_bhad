#!/usr/bin/ruby

require 'sinatra'

set :bind, '0.0.0.0'
set :port, 8080

get '/' do
    send_file File.expand_path('index.html', settings.public_folder)
end

get '/images' do
    ddd = Dir.glob('public/uploads/*.jpg')
    str = "{ \"images\": ["
    first = true
    ddd.each do |d|
        str += "," if (!first)
        first = false if (first)
        str += "\"" + d[d.index("uploads")...d.length] + "\""
    end
    str += "] }"
    response['Access-Control-Allow-Origin'] = '*'
    return str
end

# Time: Tue Sep 20 2016 17:57:17 GMT-0400 (EDT) Lat: 38.88144084 Lon: -77.11424128 Accuracy: 18
get '/locations' do
    str = "{ \"locations\": ["
    begin
        File.open 'location/current.txt', 'r' do |f|
            first = true
            f.each_line do |line|
                time = line.index('Time: ')
                lat = line.index('Lat: ')
                lon = line.index('Lon: ')
                acc = line.index('Accuracy: ')
                str += "," if (!first)
                first = false if (first)
                str += "{ \"timestamp\":\"#{line[time+6...lat-1]}\", \"coords\":[#{line[lat+5...lon-1]},#{line[lon+5...acc-1]}] }"
            end
        end
    rescue
        error 500, "Error reading locations."
    end
    str += "] }"
    response['Access-Control-Allow-Origin'] = '*'
    return str
end

# Reference: http://www.wooptoot.com/file-upload-with-sinatra
post '/upload' do
    begin
        puts params
        puts params['data'][:filename]
        puts File.absolute_path(params['data'][:tempfile].path)
        File.open('public/uploads/' + params['data'][:filename], "w") do |f|
            f.write(params['data'][:tempfile].read)
        end
    rescue => e
        puts "!!! " + e.message
        error 500, "Error uploading file."
    end
    return "Success."
end

post '/location' do
    begin
        error 500 if params[:data].class != String
        File.open 'location/current.txt', 'a' do |f|
            f.puts params[:data]
        end
    rescue
        error 500
    end
end

