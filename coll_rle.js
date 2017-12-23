/* RLE compression for a byte array of level data */

const args = process.argv.slice(2);
const mapName = args[0];
if ( !mapName ) {
	console.error('Error: no map name specified');
	process.exit(1);
}

const MAX_COL_LENGTH = 40;
const filename = `${mapName}.csv`;
const fs = require('fs');
const _ = require('lodash');
const fileContents = fs.readFileSync(filename);
const fileString = fileContents.toString();
const fileArray = fileString.split('\n');
let fileData = [];


_.each(fileArray, (line) => {
	if ( line.length ) {
		const lineData = line.split(',');
		_.each(lineData, (byte) => {
			if ( byte.length ) {
				fileData.push(parseInt(byte, 10));
			}
		});
	}
});

// break down fileData into sub-arrays of runs of identical values

let runsArray = [];
let currentRun = [];

_.each(fileData, (byte) => {
	if ( currentRun.length ) {
		if ( byte === _.head(currentRun) ) {
			currentRun.push(byte);
		} else {
			runsArray.push(currentRun);
			currentRun = [ byte ];
		}
	} else {
		currentRun.push(byte);
	}
});

let compressedData = [];

_.each(runsArray, (run) => {
	compressedData.push(_.head(run));
	compressedData.push(run.length);
});

let output = `const unsigned char ${mapName}[] = {\n\t`;
let colCount = 0;
_.each ( compressedData, (byte) => {
	let outString = byte + ', ';
	output += outString;
	colCount += outString.length;
	if ( colCount > MAX_COL_LENGTH ) {
		colCount = 0;
		output += '\n\t';
	}
});
output = _.trimEnd(output, /\n\t \,/);
output += '\n};\n';

const rleFile = `${mapName}_rle.h`;
const saved = Math.round((1 - (parseFloat(output.length) / parseFloat(fileString.length))) * 100);
fs.writeFileSync(rleFile, output);

console.log(`Wrote compressed collision map to ${rleFile} (saved ${saved}%)`);

