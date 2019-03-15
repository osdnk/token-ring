const dgram = require('dgram');
const fs = require('fs');

const server = dgram.createSocket({ type: 'udp4', reuseAddr: true });

server.on('error', (err) => {
    console.log(`server error:\n${err.stack}`);
server.close();
});

var log_file = fs.createWriteStream(__dirname + '/log.txt', {flags : 'w'});

server.on('message', (msg, rinfo) => {
    console.log(`server got: ${msg} from ${rinfo.address}:${rinfo.port}`);
    log_file.write(msg + ' ' + new Date().toUTCString() +'\n');
});

server.on('listening', () => {
    const address = server.address();
console.log(`server listening ${address.address}:${address.port}`);
});

server.bind({
   // address: '224.0.0.1',
    port: 9010,
}, () => server.addMembership("237.0.0.1"));