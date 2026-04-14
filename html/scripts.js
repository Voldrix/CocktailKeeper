const getCookie = (cookie) => (document.cookie.match('(^|;)\\s*'+cookie+'\\s*=\\s*([^;]+)')?.pop()||'');
const delCookie = (cookie) => (document.cookie = cookie + '=0; max-age=0');
var drinkid, newDrinkID, drink, newImg, imgRemoved, imgUpXhr = [], imgUpStatus = [], pixelCacheBust = [], liquors = {}, metaTags = {};
var user = getCookie('user');
var bar = location.pathname.split('/')[1] || user;
var glassIconAR = {'Beer Mug':0.851, Coffee:0.685, Collins:0.33, 'Copper Mug':0, Cosmopolitan:0.939, Coupe:0.717, Flute:0.328, Goblett:0.641, Highball:0.467, Hurricane:0.432, Margarita:0.72, Martini:0.683, Milkshake:0.417, 'Old Fashioned':0.85, Pint:0.613, 'Red Wine':0.478, Rocks:0.845, Shot:0.636, Snifter:0.658, 'Tiki Mug':0.452, Tulip:0.44, 'White Wine':0.357};

if(bar) getBar(bar);
else route('login');


//POP STATE
window.onkeyup = (e) => {if(e.key === 'Escape') history.back()};
window.onpopstate = function(event) {
  if(event.state) {
    switch(event.state) {
      case 'bar': route('home'); break;
      case 'edit': editRecipe(true); break;
      case 'new': newRecipe(true); break;
      case 'auth': route('login'); break;
      default: viewRecipe(event.state, true);
    }
  }
  else
    route('home');
};


//ROUTE
function route(_route) {
  editRecipeBox.style.display = 'none';
  viewRecipeBox.style.display = 'none';
  authContainer.style.display = 'none';
  recipeContainer.style.display = 'none';
  controlBar.style.display = 'block';
  controlBarMain.style.display = 'none';
  controlBarView.style.display = 'none';
  controlBarEdit.style.display = 'none';
  newImg = '';
  user = getCookie('user');
  if(imgUpXhr[drinkid || newDrinkID] && imgUpStatus[drinkid || newDrinkID] !== 'saved') {
    imgUpXhr[drinkid || newDrinkID].abort();
    imgUpXhr[drinkid || newDrinkID] = null;
    imgUpStatus[drinkid || newDrinkID] = 'canceled';
  }

  if(_route === 'home') {
    document.title = bar + "'s Bar | Cocktail Keeper";
    recipeContainer.style.display = 'flex';
    controlBarMain.style.display = 'block';
    newDrinkBtn.style.display = (!user || bar.toLowerCase() !== user.toLowerCase()) ? 'none' : null;
  }
  if(_route === 'login') {
    authContainer.style.display = 'block';
    controlBar.style.display = 'none';
    if(user) {
      loginContainer.style.display = 'none';
      logoutContainer.style.display = 'flex';
      logoutUN.textContent = 'User: ' + user;
    }
    else {
      loginContainer.style.display = 'flex';
      logoutContainer.style.display = 'none';
    }
    if(history.state !== 'auth') {
      if(history.state) history.pushState('auth', '', '/');
      else history.replaceState('auth', '', '/');
      document.title = 'Cocktail Keeper';
    }
  }
  if(_route === 'view') {
    viewRecipeBox.style.display = 'block';
    controlBarView.style.display = 'flex';
    if(user && user.toLowerCase() === bar.toLowerCase()) {
      editRecipeBtn.style.display = 'inline-block';
      cloneRecipeBtn.style.display = 'none';
    }
    else {
      editRecipeBtn.style.display = 'none';
      cloneRecipeBtn.style.display = 'inline-block';
    }
  }
  if(_route === 'edit') {
    editRecipeBox.style.display = 'block';
    controlBarEdit.style.display = 'flex';
  }
}


function home() {
  event.preventDefault();
  if(getCookie('user')) {
    if(user.toLowerCase() !== bar.toLowerCase()) {
      location.href = '/';
    }
    else if(history.state !== 'bar') {
      history.pushState('bar', '', '/' + bar);
      route('home');
    }
  }
  else {
    route('login');
  }
}


//GET BAR
function getBar(_bar) {
  var re = /^[0-9a-zA-Z_\-]+$/;
  if(!re.test(_bar)) {
    barName.textContent = 'Error: Invalid Bar Name';
    recipeContainer.innerHTML = '';
    route('home');
    return;
  }
  const drinkHotLink = location.pathname.split('/')[2];
  history.replaceState('bar', '', '/'+_bar);
  barName.textContent = _bar + "'s bar";

  var xhttp = new XMLHttpRequest();
  xhttp.onloadend = function() {
    if(this.status === 200) {
      var recipes = JSON.parse(this.responseText);
      liquors = {}; metaTags = {};
      filterLiquor.innerHTML = '<option value="">Liquor</option>';
      filterTag.innerHTML = '<option value="">Tag</option>';

      renderDrinks(recipes, false);

      liquors = Object.keys(liquors).sort((a,b) => a.localeCompare(b,'en',{sensitivity:'base'}));
      for(liquor of liquors) { //filter liquor select
        let option = document.createElement('option');
        option.text = option.value = liquor;
        filterLiquor.appendChild(option);
      }
      metaTags = Object.keys(metaTags).sort((a,b) => a.localeCompare(b,'en',{sensitivity:'base'}));
      for(tag of metaTags) { //filter + edit meta select
        let option = document.createElement('option');
        option.text = option.value = tag;
        editMetaTag.appendChild(option.cloneNode(true));
        filterTag.appendChild(option);
      }
    }
    else recipeContainer.textContent = "Error: " + this.status + " \n" + this.responseText;
  }

  xhttp.open('GET', '/api/getBar/' + encodeURIComponent(_bar), true);
  xhttp.send();
  if(drinkHotLink)
    viewRecipe(drinkHotLink, false);
  else
    route('home');
}


//RENDER DRINKS
function renderDrinks(recipes, prepend) {
  if(!prepend)
    recipeContainer.innerHTML = '';

  for(_drink of recipes) {
    let drinkTile = document.createElement('a'); //tile
    drinkTile.href = '/' + bar + '/' + _drink.drinkid + '#' + slugify(_drink.name);
    drinkTile.classList.add('drinkTile');
    drinkTile.setAttribute('mdate', _drink.mdate); //mod date
    drinkTile.setAttribute('primaryLiquor', _drink.primaryLiquor); //prime liquor
    if(_drink.primaryLiquor) liquors[_drink.primaryLiquor] = 1;
    drinkTile.setAttribute('onclick', "viewRecipe('"+_drink.drinkid+"',false)"); //click to view
    drinkTile.setAttribute('draggable', 'false');
    drinkTile.id = _drink.drinkid; //id

    if(_drink.img === '1') { //img
      let drinkImage = document.createElement('img'); //image
      drinkImage.src = newImg || '/img/' + bar.toLowerCase() + '/' + _drink.drinkid + '.png';
      if(_drink.imgRes) {
        let imgRes = _drink.imgRes.split('x');
        drinkImage.height = 156;
        drinkImage.width = (156 / imgRes[1]) * imgRes[0];
      }
      drinkTile.appendChild(drinkImage);
    }

    let drinkInfo = document.createElement('div'); //text wrapper
    drinkInfo.classList.add('drinkInfoWrapper');
    drinkTile.appendChild(drinkInfo);

    let drinkName = document.createElement('div'); //name
    drinkName.classList.add('drinkName');
    drinkName.textContent = _drink.name;
    drinkInfo.appendChild(drinkName);
    let drinkIngr = document.createElement('div'); //ingredients
    drinkIngr.classList.add('drinkIngredientList');
    if(_drink.summary) {
      drinkIngr.textContent = _drink.ingredientList;
      drinkIngr.style.whiteSpace = 'pre-wrap';
    }
    else drinkIngr.textContent = _drink.ingredientList.replaceAll(', ', "\n");
    if(_drink.summary) {
      let drinkSummary = document.createElement('div'); //summary
      drinkSummary.classList.add('drinkSummary');
      drinkSummary.textContent = _drink.summary;
      drinkIngr.appendChild(drinkSummary);
    }
    drinkInfo.appendChild(drinkIngr);

    let tags = _drink.tags?.split(',');
    if(_drink.glass || (tags && tags != 0 && tags[0] != '')) { //glass & tags

      let glassAndTags = document.createElement('div');
      glassAndTags.classList.add('glassAndTags');

      if(_drink.glass) { //glass
        let drinkGlass = document.createElement('img');
        drinkGlass.src = '/img/glass/' + _drink.glass + '.svg';
        drinkGlass.style.width = (glassIconAR[_drink.glass] * 27) + 'px';
        glassAndTags.appendChild(drinkGlass);
      }

      if(tags && tags != 0 && tags[0] != '') { //tags
        let meta = document.createElement('div'); //meta tags
        meta.classList.add('tileMetaTags');
        for(tag of tags) {
          let spn = document.createElement('span');
          spn.classList.add('tileTag');
          spn.textContent = tag;
          meta.appendChild(spn);
          metaTags[tag] = 1;
        }
        glassAndTags.appendChild(meta);
      }

      drinkInfo.appendChild(glassAndTags);
    }

    if(prepend)
      recipeContainer.insertAdjacentElement('afterbegin', drinkTile);
    else
      recipeContainer.appendChild(drinkTile);
  }
}


//EDIT RECIPE
function editRecipe(popState) {
  if(!getCookie('user')) {
    route('login');
    return;
  }
  if(user.toLowerCase() !== bar.toLowerCase()) return;
  clearEdit();
  editDrinkTitle.textContent = 'Edit Drink';
  editImg.src = (drink.img === '1') ? '/img/' + bar.toLowerCase() + '/' + drinkid + '.png' + (pixelCacheBust[drinkid] ? '?v=' + pixelCacheBust[drinkid] : '') : '';
  editName.value = drink.name;
  editGarnish.value = drink.garnish;
  editSummary.value = drink.summary;
  editInstructions.value = drink.instructions;
  editGlass.value = drink.glass || '';
  editGlassImg.src = drink.glass ? '/img/glass/' + drink.glass + '.svg' : '';
  for(ingr of JSON.parse(drink.ingredients)) {
    let lastRow = editIngredients.lastElementChild;
    editIngredients.appendChild(lastRow.cloneNode(true));
    for(let i = 0; i < 4; i++)
      lastRow.children[i].value = ingr[i];
  }
  let tags = drink.tags?.split(',');
  if(tags && tags != 0 && tags[0] != '') {
    for(tag of tags) {
      let et = document.createElement('div');
      et.classList.add('editTag');
      et.setAttribute('onclick', "deleteMetaTag(this)");
      et.textContent = tag;
      editMetaTags.appendChild(et);
    }
  }
  route('edit');
  editSummary.style.height = editSummary.scrollHeight - 5 + 'px';
  editInstructions.style.height = editInstructions.scrollHeight - 5 + 'px';
  if(!popState)
    history.pushState('edit', '', null);
}


//VIEW RECIPE
function viewRecipe(did, popState) {
  if(event) event.preventDefault();
  if(window.getSelection().anchorOffset - window.getSelection().focusOffset) //allow highlight card/tile w/o viewing
    return;

  if(!popState)
    history.pushState(did, '', '/' + bar + '/' + did);

  var xhttp = new XMLHttpRequest();
  xhttp.onloadend = renderViewDrink;
  function renderViewDrink() {
    if(!this.status || this.status === 200) {
      drinkid = did;
      if(this.responseText) drink = JSON.parse(this.responseText);
      viewName.textContent = drink.name;
      viewSummary.textContent = drink.summary;
      viewGarnish.textContent = drink.garnish;
      viewInstructions.textContent = drink.instructions;
      viewGlass.textContent = drink.glass;
      glassImg.src = drink.glass ? '/img/glass/' + drink.glass + '.svg' : '';
      if(drink.img === '1')
        drinkImg.src = newImg || '/img/' + bar.toLowerCase() + '/' + did + '.png' + (pixelCacheBust[drinkid] ? '?v=' + pixelCacheBust[drinkid] : '');
      else drinkImg.src = '';
      viewIngredients.innerHTML = '';
      for(ingr of JSON.parse(drink.ingredients)) {
        let row = viewIngredients.insertRow();
        let cel = row.insertCell(); cel.textContent = ingr[0];
        cel = row.insertCell(); cel.textContent = ingr[1];
        cel = row.insertCell(); cel.textContent = ingr[2];
        cel = row.insertCell(); cel.textContent = ingr[3];
      }
      viewTags.textContent = drink.tags?.replaceAll(',',', ');

      if(!popState)
        history.replaceState(did, '', '/' + bar + '/' + did + '#' + slugify(drink.name));
      document.title = drink.name + " | Cocktail Keeper";
      json_ld();
    }
    else alert('Error getting drink: ' + did + ' - ' + this.status);
  }

  if((drinkid || newDrinkID) === did) { //same drink
    if(drink?.drinkid == did) { //drink just edited
      renderViewDrink();
      delete drink.drinkid;
    }
    else {
      history.replaceState(did, '', '/' + bar + '/' + did + '#' + slugify(drink.name));
      document.title = drink.name + " | Cocktail Keeper";
    }
  }
  else { //diff drink
    xhttp.open('GET', '/api/getDrink/' + bar + '/' + did, true);
    xhttp.send();
  }
  route('view');
}


//ADD or SAVE RECIPE
function saveRecipe() {
  saveRecipeBtn.disabled = true;
  const date = new Date();
  const dateTime = date.toISOString().replace('T', ' ').split('.')[0];
  var tags = [];
  for(x of editMetaTags.children)
    tags.push(x.textContent);

  var primLiquor = editIngredients.querySelector('.IngredientText').value;
  var ingrList = Array.from(editIngredients.querySelectorAll('.IngredientText')).flatMap(e => e.value || []).join(', ');
  var ingrRows = Array.from(editIngredients.children).map(e => e.children);
  var imgBool = ((drinkid && drink.img === '1') || (!!imgFile.value)) ? '1' : '0';
  var imgTrigger = '0';

  if(!editName.value || !primLiquor || !ingrList) {
    saveRecipeBtn.disabled = false;
    alert("Name and at least one ingredient are required");
    return;
  }

  if(drinkid && drink.img === '1' && imgRemoved && !imgFile.value) imgBool = '2';
  if(imgFile.value) { //new img
    if(imgUpStatus[drinkid || newDrinkID] === 'done') { //img up done b4 save
      setTimeout(()=>{fetch('/img/' + bar.toLowerCase() + '/' + (drinkid || newDrinkID) + '.png', {cache: "reload"});}, 2000);
      pixelCacheBust[drinkid || newDrinkID] = (pixelCacheBust[drinkid || newDrinkID] || 0) + 1;
      imgTrigger = '1';
    }
    imgUpStatus[drinkid || newDrinkID] = 'saved';
  }

  var ingrArr = [];
  for(ingrRow of ingrRows) {
    if(!ingrRow[2].value.trim()) continue;
    ingrArr.push([ingrRow[0].value, ingrRow[1].value, ingrRow[2].value, ingrRow[3].value]);
  }
  ingrArr = JSON.stringify(ingrArr);

  var data = {'drinkid': (drinkid || newDrinkID), 'name': editName.value, 'img': imgBool, 'mdate': dateTime, 'primaryLiquor': primLiquor, 'ingredientList': ingrList, 'glass': editGlass.value, 'tags': tags.join(','), 'summary': editSummary.value, 'ingredients': ingrArr, 'instructions': editInstructions.value, 'garnish': editGarnish.value};

  var fd = new FormData();
  fd.set('drinkid', (drinkid || newDrinkID));
  fd.set('img', imgBool);
  fd.set('imgTrigger', imgTrigger);
  fd.set('name', editName.value);
  fd.set('garnish', editGarnish.value);
  fd.set('summary', editSummary.value);
  fd.set('ingredients', ingrArr);
  fd.set('instructions', editInstructions.value);
  fd.set('glass', editGlass.value);
  fd.set('ingredientList', ingrList);
  fd.set('primaryLiquor', primLiquor);
  fd.set('tags', tags.join(','));

  var xhttp = new XMLHttpRequest();
  xhttp.onloadend = function() {
    if(this.status === 200) {
      if(drinkid) //edit
        document.getElementById(drinkid).remove();
      drink = data;
      renderDrinks([data], true);
      history.back();
    }
    else
      alert('Error: ' + this.status + "\nFailed to save\n" + this.responseText);
    saveRecipeBtn.disabled = false;
  }

  if(drinkid) xhttp.open('POST', '/api/edit/drink', true);
  else xhttp.open('POST', '/api/add/drink', true);
  xhttp.send(fd);
}


//IMAGE UPLOAD
function imgUpload() {
  var did = (drinkid || newDrinkID), drinkImg = drink?.img;

  if(imgUpXhr[did]) {
    imgUpXhr[did].abort();
    imgUpXhr[did] = null;
    imgUpStatus[did] = 'canceled';
    editImg.src = (drinkImg === '1') ? '/img/' + bar.toLowerCase() + '/' + did + '.png' : '';
  }
  if(!imgFile.value) return;
  if(imgFile.files[0].size < 384 || imgFile.files[0].size > 25165824) {
    imgFile.value = null;
    alert('Image Size Limit: 24MB');
    return;
  }

  var newLocalImg = new Image();
  var localImgFile = URL.createObjectURL(imgFile.files[0]);

  newLocalImg.onerror = () => {
    imgFile.value = null;
    imgUpXhr[did] = null;
    alert('Invalid Image');
  }

  newLocalImg.onload = () => {
    imgUpXhr[did].open('POST', '/api/imageUpload/' + did, true);
    imgUpXhr[did].send(imgFile.files[0]);
    imgUpStatus[did] = 'uploading';
    newImg = editImg.src = localImgFile;
  }

  imgUpXhr[did] = new XMLHttpRequest();
  imgUpXhr[did].onloadend = function() {
    if(this.status === 200) { //img up success
      if(imgUpStatus[did] === 'saved') { //saved before upload finished
        fetch('/api/imgTrigger/' + did);
        setTimeout(()=>{fetch('/img/' + bar.toLowerCase() + '/' + did + '.png', {cache: "reload"});}, 2500);
        pixelCacheBust[did] = (pixelCacheBust[did] || 0) + 1;
      }
      imgUpStatus[did] = 'done';
    }

    else { //img upload fail
      //revert local images
      if(imgUpStatus[did] === 'saved') { //saved before upload finished
        var drinkTileImg = document.getElementById(drinkid);
        if(drinkTileImg) { //tile
          drinkTileImg = drinkTileImg.querySelector('img');
          if(drinkTileImg) //tile img
            drinkTileImg.src = (drinkImg === '1') ? '/img/' + bar.toLowerCase() + '/' + did + '.png' : '';
        }
        if(drinkImg.getAttribute('src') === localImgFile) { //view
          drinkImg.src = (drinkImg === '1') ? '/img/' + bar.toLowerCase() + '/' + did + '.png' : '';
          drinkTileImg.src = (drinkImg === '1') ? '/img/' + bar.toLowerCase() + '/' + did + '.png' : '';
        }
        fetch('/api/imgTrigger/' + did);
      }

      if(imgUpStatus[did] === 'uploading') //not saved or canceled yet
        newImg = editImg.src = ''; imgFile.value = null;

      imgUpStatus[did] = 'failed';
      URL.revokeObjectURL(localImgFile);
      if(this.status)
        alert('Image Upload Error: ' + this.status + "\n" + this.responseText);
    }

    imgUpXhr[did] = null;
  }

  newLocalImg.src = localImgFile;
}


//REMOVE IMG
function removeImg() {
  event.preventDefault();
  const did = drinkid || newDrinkID;
  if(imgUpXhr[did]) {
    imgUpXhr[did].abort();
    imgUpXhr[did] = null;
    imgUpStatus[did] = 'canceled';
  }
  imgRemoved = 1;
  imgFile.value = null;
  editImg.src = '';
}


//NEW RECIPE
function newRecipe(popState) {
  if(!getCookie('user')) {
    route('login');
    return;
  }
  if(user.toLowerCase() !== bar.toLowerCase()) return;
  drink = null;
  drinkid = 0;
  newDrinkID = genID();
  clearEdit();
  editDrinkTitle.textContent = 'New Drink';
  route('edit');
  if(!popState)
    history.pushState('new', '', null);
}


//ADD META TAG
function addMetaTag() {
  var _tag = editMetaTag.value;
  editMetaTag.value = '';
  var uniq = 0;
  for(x of editMetaTags.children)
    uniq += (x.textContent === _tag);
  if(!uniq) {
    let et = document.createElement('div');
    et.classList.add('editTag');
    et.setAttribute('onclick', "deleteMetaTag(this)");
    et.textContent = _tag;
    editMetaTags.appendChild(et);
  }
}


//ADD NEW META TAG
function addNewMetaTag() {
  var _tag = editNewMetaTag.value.trim();
  var uniq = 0;
  for(x of editMetaTags.children)
    uniq += (x.textContent === _tag);
  if(!_tag || uniq) return;
  if(_tag.length > 48) {
    alert('Meta Tag size limit 48 characters');
    return;
  }
  if(!/^[0-9a-zA-Z _\.\-]+$/.test(_tag)) {
    alert("No special characters in Meta Tags\nExcept . _ -");
    return;
  }
  let et = document.createElement('div');
  et.classList.add('editTag');
  et.setAttribute('onclick', "deleteMetaTag(this)");
  et.textContent = _tag;
  editMetaTags.appendChild(et);

  editNewMetaTag.value = null;
  for(x of editMetaTag.children)
    uniq += (x.value === _tag);
  if(!uniq) {
    let option = document.createElement('option');
    option.text = option.value = _tag;
    editMetaTag.appendChild(option.cloneNode(true));
    filterTag.appendChild(option);
  }
}


//DELETE META TAG
function deleteMetaTag(_tag) {
  if(confirm('DELETE Tag: ' + _tag.textContent + '?')) {
    _tag.remove();
  }
}


//NEXT INGREDIENT
function nextIngredient(elem) {
  if(elem.value) { //add row
    if(!elem.parentElement.nextElementSibling) { //last row
      var newRow = elem.parentElement.cloneNode(true);
      for(field of newRow.children)
        field.value = '';
      editIngredients.appendChild(newRow);
    }
  }
  else { //remove rows
    var delCurRow = 0;
    var lastRow = editIngredients.lastElementChild;
    while(!lastRow.querySelector('.IngredientText').value && lastRow.previousElementSibling && !lastRow.previousElementSibling.querySelector('.IngredientText').value) {
      delCurRow += (elem.parentElement === lastRow);
      lastRow.remove();
      lastRow = editIngredients.lastElementChild;
    }
    if(delCurRow) lastRow.querySelector('.IngredientText').select();
  }
}


//FILTER
function filter() {
  filterLiquor.blur();
  filterTag.blur();
  var liquor = filterLiquor.value;
  var tag = filterTag.value;
  var search = searchbox.value.toLowerCase();
  for(drink of recipeContainer.children) {
    let hide = 3;
    let pl = drink.getAttribute('primaryliquor');
    hide -= (!liquor || pl === liquor);
    if(hide === 2) {
      if(!tag) hide -= 1;
      else {
        for(mt of drink.getElementsByClassName('tileTag')) {
          hide -= (mt.textContent === tag);
        }
      }
    }
    if(hide === 1) {
      if(!search) hide -= 1;
      else hide -= drink.textContent.toLowerCase().includes(search);
    }
    drink.style.display = hide ? 'none' : 'flex';
  }
}


//SORT
function sort(_sort) {
  sortSelect.blur();
  if(!_sort) return;

  var tiles = Array.from(recipeContainer.children);

  if(_sort === 'alphabetical')
    tiles.sort((a, b) => +(a.textContent > b.textContent) || -1);
  if(_sort === 'newest')
    tiles.sort((a, b) => +(a.getAttribute('mdate') < b.getAttribute('mdate')) || -1);
  if(_sort === 'oldest')
    tiles.sort((a, b) => +(a.getAttribute('mdate') > b.getAttribute('mdate')) || -1);

  tiles.forEach(item => {recipeContainer.appendChild(item);});
}


//CLONE RECIPE
function cloneRecipe() {
  if(!getCookie('user')) {
    alert('Login on the home page to clone recipe');
    return;
  }
  if(!drinkid) {alert('Clone Failed'); return;}

  var xhttp = new XMLHttpRequest();
  xhttp.onloadend = function() {
    if(this.status === 200) {
      alert("Drink Cloned!\n" + viewName.textContent);
    }
    else
      alert('Clone Failed. Error: ' + this.status + "\n" + this.responseText);
    cloneRecipeBtn.disabled = false;
  }

  cloneRecipeBtn.disabled = true;
  xhttp.open('GET', '/api/clone/' + bar + '/' + drinkid, true);
  xhttp.send();
}


//DELETE RECIPE
function deleteRecipe() {
  if(!drinkid) {history.back(); return;}

  var xhttp = new XMLHttpRequest();
  xhttp.onloadend = function() {
    if(this.status === 200) {
      document.getElementById(drinkid).remove();
      history.replaceState('bar', '', '/' + bar);
      route('home');
    }
    else
      alert('Error: ' + this.status + "\nFailed to delete\n" + this.responseText);
    deleteRecipeBtn.disabled = false;
  }

  if(confirm('DELETE ' + viewName.textContent + '?')) {
    deleteRecipeBtn.disabled = true;
    if(imgUpXhr[drinkid]) {
      imgUpXhr[drinkid].abort();
      imgUpXhr[drinkid] = null;
      imgUpStatus[drinkid] = 'deleted';
    }
    xhttp.open('GET', '/api/delete/' + drinkid, true);
    xhttp.send();
  }
}


//LOGIN
function login() {
  loginBtn.disabled = true;
  registerBtn.disabled = true;
  var _un = un.value;
  var _pw = pw.value;
  loginErrMsg.innerText = '';

  if(_un.length < 1 || _un.length > 31) {
    loginErrMsg.innerText = 'Username must be 1 - 31 characters';
  }
  if(_pw.length < 1 || _pw.length > 127) {
    if(loginErrMsg.innerText.length > 1) loginErrMsg.innerHTML += '<br>';
    loginErrMsg.innerHTML += 'Password must be 1 - 127 characters';
  }
  var re = /^[a-zA-Z0-9\-_]+$/;
  if(!re.test(_un)) {
    if(loginErrMsg.innerText.length > 1) loginErrMsg.innerHTML += '<br>';
    loginErrMsg.innerHTML += 'Username allowed characters: a-z A-Z 0-9 -_';
  }

  if(loginErrMsg.innerText.length > 1) {
    loginBtn.disabled = false;
    registerBtn.disabled = false;
    return;
  }

  //send login request
  var xhttp = new XMLHttpRequest();
  xhttp.onloadend = function() {
    if(this.status === 200) { //login success
      bar = user = getCookie('user');
      history.pushState('bar', '', '/'+bar);
      getBar(user);
    }
    else {
      loginErrMsg.textContent = this.responseText;
    }
    loginBtn.disabled = false;
    registerBtn.disabled = false;
  }

  xhttp.open('POST', '/api/login/' + _un, true);
  xhttp.send(_pw);
}


//REGISTER
function register() {
  loginBtn.disabled = true;
  registerBtn.disabled = false;
  var _un = un.value;
  var _pw = pw.value;
  loginErrMsg.innerText = '';

  if(_un.length < 1 || _un.length > 31) {
    loginErrMsg.innerText = 'Username must be 1 - 31 characters';
  }
  if(_pw.length < 1 || _pw.length > 127) {
    if(loginErrMsg.innerText.length > 1) loginErrMsg.innerHTML += '<br>';
    loginErrMsg.innerHTML += 'Password must be 1 - 127 characters';
  }

  var re = /^[0-9a-zA-Z_\-]+$/;
  if(!re.test(_un)) {
    if(loginErrMsg.innerText.length > 1) loginErrMsg.innerHTML += '<br>';
    loginErrMsg.innerHTML += 'Username allowed characters: 0-9 A-Z a-z -_';
  }

  if(loginErrMsg.innerText.length > 1) {
    loginBtn.disabled = false;
    registerBtn.disabled = false;
    return;
  }

  var xhttp = new XMLHttpRequest();
  xhttp.onloadend = function() {
    if(this.status === 200) {
      bar = user = getCookie('user');
      history.pushState('bar', '', '/'+bar);
      getBar(user);
    }
    else {
      loginErrMsg.textContent = this.responseText;
    }
    loginBtn.disabled = false;
    registerBtn.disabled = false;
  }

  xhttp.open('POST', '/api/register/' + _un, true);
  xhttp.send(_pw);
}


//SHARE
function shareLink() {
  if(navigator.clipboard) { //ssl
    navigator.clipboard.writeText(window.location.href);
  }
  else { //http
    var textArea = document.createElement("textarea");
    textArea.value = window.location.href;
    textArea.style.top = "0";
    textArea.style.left = "0";
    textArea.style.opacity = '0';
    textArea.style.position = "fixed";
    document.body.appendChild(textArea);
    textArea.focus();
    textArea.select();
    var successful = document.execCommand('copy');
    document.body.removeChild(textArea);
    if(!successful) return;
  }
  shareLinkNotice.style.display = 'block';
  setTimeout(()=>{shareLinkNotice.style.display = 'none'}, 3000);
}


//LOGOUT
function logout() {
  user = null;
  delCookie('user');
  delCookie('id');
  delCookie('tok');
}


//SEARCH UI
function searchOpen(close=false) {
  if(!close) {
    searchbox.style.opacity = 1;
    searchbox.style.width = '160px';
    searchbox.style.borderColor = 'white';
    searchbox.style.pointerEvents = 'all';
    searchicon.style.opacity = 0;
    searchicon.style.pointerEvents = 'none';
    searchbox.focus();
  }
  else {
    searchbox.style.width = 0;
    searchbox.style.borderColor = 'var(--accent)';
    searchbox.style.pointerEvents = 'none';
    searchicon.style.opacity = 1;
    searchicon.style.pointerEvents = 'all';
  }
}


//CLEAR EDIT
function clearEdit() {
  imgRemoved = 0;
  editImg.src = '';
  imgFile.value = null;
  editName.value = null;
  editGarnish.value = null;
  editSummary.value = null;
  editInstructions.value = null;
  editMetaTags.innerHTML = '';
  editGlass.value = '';
  editGlassImg.src = '';
  editMetaTag.value = '';
  editNewMetaTag.value = null;
  editSummary.style.height = null;
  editInstructions.style.height = null;
  var lastRow = editIngredients.lastElementChild;
  while(lastRow.previousElementSibling) {
    lastRow.previousElementSibling.remove();
  }
  for(field of lastRow.children)
    field.value = '';
}


//SLUGIFY
function slugify(text) {
  return text
  .replace(/[^a-zA-Z0-9 -]/g, '')
  .replace(/\s+/g, '-')
  .replace(/-+/g, '-');
}


//ID GEN
function genID() {
  const array = new Uint8Array(16);
  crypto.getRandomValues(array);
  for(let i = 0; i < 16; i++) {
    array[i] &= 31;
    array[i] += (array[i] > 5) ? 91 : 52;
  }
  return String.fromCharCode(...array);
}


//JSON-LD
function json_ld() {
  if(document.getElementById('jsonld')) document.getElementById('jsonld').remove();
  var jsonld = document.createElement('script');
  jsonld.setAttribute('type', 'application/ld+json');
  jsonld.id = 'jsonld';
  var rr = {
    "@context": "https://schema.org/",
    "@type": "Recipe",
    "author": {
      "@type": "Person",
      "name": bar
    },
    "datePublished": drink.cdate,
    "description": drink.summary,
    "image": (drink.img === '1') ? '/img/' + bar.toLowerCase() + '/' + (drinkid || newDrinkID) + '.png' : null,
    "recipeIngredient": drink.ingredients,
    "name": drink.name,
    "recipeInstructions": drink.instructions?.split("\n"),
    "publisher": {
      "@type": "Organization",
      "name": "CocktailKeeper",
      "logo": 'https://cocktailkeeper.cc/favicon.ico'
    }
  }
  jsonld.innerText = JSON.stringify(rr);
  document.head.appendChild(jsonld);
}

