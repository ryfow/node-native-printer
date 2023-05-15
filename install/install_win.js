const fs = require('fs');
const inquirer = require('inquirer');
const dotenv = require('dotenv');

module.exports = function(){
	makeEnv("edge");
};

function makeEnv(pack) {
	fs.writeFileSync(fs.realpathSync(__dirname + '\\..') + "\\.env", `NNP_PACKAGE=${pack}`);
}

