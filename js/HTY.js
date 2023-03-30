// get the width and height of window
const windowWidth = window.innerWidth;
const windowHeight = window.innerHeight;
var subPageName;
//some enlarge and ensmall
document.getElementsByClassName("product_image")[0].style.width = windowWidth * 0.6 +"px" //just 4:3
document.getElementsByClassName("product_image")[0].style.height = windowWidth * 0.45 +"px"
document.getElementsByClassName("product_header")[0].style.fontSize = 250 * windowWidth / 1440 +"%"
document.getElementsByClassName("product_description")[0].style.fontSize = 100 * windowWidth / 1440 +"%"
document.getElementsByClassName("product_description")[0].style.fontSize = 120 * windowWidth / 1440 +"%"
document.getElementsByClassName("story_slogan_description")[0].style.fontSize = 200 * windowWidth / 1440 +"%"
document.getElementsByClassName("generalStructure_image")[0].style.width = windowWidth * 0.68 +"px" //just 16:10
document.getElementsByClassName("generalStructure_image")[0].style.height = windowWidth * 0.425 +"px"
var synchr = function()
{
	subPageName = window.location.hash;
	if (document.getElementById("CH_EN").innerHTML == "CH/EN")
		document.getElementById("CH_EN").href = "Hutao Shake Toy_ch.html" + window.location.hash;
	else
		document.getElementById("CH_EN").href = "Hutao Shake Toy.html" + window.location.hash;
}
window.onload = function() {
  synchr();
};
