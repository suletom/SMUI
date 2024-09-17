var express = require('express');
const bodyParser = require('body-parser');
let fs = require('fs');

var app = express();

app.use(bodyParser.json());

let t = "";

try {
    t = fs.readFileSync('template.html', { encoding: 'utf8', flag: 'r' });
} catch (e) {
    console.log(e);
}


app.get('/', function (req, res) {

    let sc = "";

    try {
        sc = fs.readFileSync('dash.html', { encoding: 'utf8', flag: 'r' });
    } catch (e) {
        console.log(e);
    }

    let d = t;
    d=d.replace("<!--sc-->", sc);
    res.send(d);

});

app.get('/ajax', function (req, res) {

    let d = "dyn cont main";

    res.send(d);

});

app.get('/setup', function (req, res) {

    let sc = "";

    try {
        sc = fs.readFileSync('setup.html', { encoding: 'utf8', flag: 'r' });
    } catch (e) {
        console.log(e);
    }

    let d = t;
    d=d.replace("<!--sc-->", sc);
    res.send(d);

});

app.get('/setup/ajax', function (req, res) {
    let d="dyn cont setup";
    res.send(d);

});

app.listen(6789, function () {

    console.log("Web ui running on port 6789! Access: http://localhost:6789");

});