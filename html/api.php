<?php
$urlPath = explode('/', $_SERVER['REQUEST_URI']);
if(count($urlPath) < 4 || empty($urlPath[3])) { http_response_code(400); echo 'Invalid Request'; exit(); }

$scon = mysqli_connect('localhost','USER','PASSWD','DATABASE');
mysqli_set_charset($scon,'utf8mb4');

switch ($urlPath[2]) {
  case 'getBar': getBar(); break;
  case 'getDrink': getDrink(); break;
  case 'login': login(); break;
  case 'register': register(); break;
  case 'delete': if(auth()) deleteDrink(); break;
  case 'add': if(auth()) saveDrink('new'); break;
  case 'edit': if(auth()) saveDrink('edit'); break;
  case 'clone': if(auth()) cloneDrink(); break;
  case 'imageUpload': if(auth()) imageUpload(); break;
  case 'imgTrigger': if(auth()) imgTrigger(false); break;
  case 'cleanImages': cleanImages(); break;
  case 'export': if(auth()) export(); break;
  default: http_response_code(400);
}

mysqli_close($scon);


function auth() {
  if(empty($_COOKIE['user']) || empty($_COOKIE['id']) || empty($_COOKIE['tok']) || (md5('XXX'.$_COOKIE['user'].$_COOKIE['id'].'XXX') !== $_COOKIE['tok'])) {
    http_response_code(401);
    echo 'User not logged in';
    return 0;
  }
  return 1;
}


function getBar() { //GET BAR
  global $scon, $urlPath;
  if(!sqlSafe($urlPath[3])) {http_response_code(400); echo 'Bad Username'; return;}

  if(isset($_COOKIE['user']) && $_COOKIE['user'] === $urlPath[3]) {
    if(!ctype_digit($_COOKIE['id'])) {http_response_code(400); echo 'Invalid Credentials'; return;}
    $id = $_COOKIE['id'];
  }
  else {
    $res = mysqli_query($scon, 'SELECT userid FROM users WHERE username="'.$urlPath[3].'" LIMIT 1');
    if(!$res) {http_response_code(404); echo 'User Not Found'; return;}
    $id = mysqli_fetch_row($res)[0];
  }

  $res = mysqli_query($scon, 'SELECT drinkid,name,img,mdate,imgRes,primaryLiquor,ingredientList,glass,tags,summary FROM drinks WHERE userid="'.$id.'"');

  if($res) echo json_encode(mysqli_fetch_all($res, MYSQLI_ASSOC));
  else echo '[]';
}


function getDrink() { //GET DRINK
  global $scon, $urlPath;
  if(empty($urlPath[4])) { http_response_code(400); echo 'Too Few Arguments'; return; }
  if(!sqlSafe($urlPath[3])) {http_response_code(400); echo 'Bad Username'; return;}
  if(!ctype_alnum($urlPath[4])) {http_response_code(400); echo 'Bad Drink ID'; return;}

  $un  = $urlPath[3];
  $did = $urlPath[4];

  if(isset($_COOKIE['user']) && $_COOKIE['user'] === $un) {
    if(!ctype_digit($_COOKIE['id'])) {http_response_code(400); echo 'Invalid Credentials'; return;}
    $id = $_COOKIE['id'];
  }
  else {
    $res = mysqli_query($scon, 'SELECT userid FROM users WHERE username="'.$un.'" LIMIT 1');
    if(!$res) {http_response_code(404); echo 'User Not Found'; return;}
    $id = mysqli_fetch_row($res)[0];
  }

  $res = mysqli_query($scon, 'SELECT name,img,cdate,mdate,primaryLiquor,ingredientList,garnish,glass,tags,summary,ingredients,instructions FROM drinks WHERE userid='.$id.' AND drinkid="'.$did.'" LIMIT 1');
  if(!$res) {http_response_code(404); echo 'Drink Not Found'; return;}
  $drink = mysqli_fetch_assoc($res);
  echo json_encode($drink);
}


function saveDrink($addEdit) { //SAVE DRINK
  global $scon;
  if(validateDrink()) {http_response_code(400); echo 'Invalid Data'; return;}

  if($_POST['img'] === '2') {
    $_POST['img'] = '0';
    unlink('img/'.strtolower($_COOKIE['user']).'/'.$_POST['drinkid'].'.png');
  }

  if($addEdit === 'new') {
    $sq = $scon->prepare('INSERT INTO drinks(userid,drinkid,name,img,primaryLiquor,ingredientList,garnish,glass,tags,summary,ingredients,instructions) VALUES(?,?,?,?,?,?,?,?,?,?,?,?)');
    $sq->bind_param('ssssssssssss', $_COOKIE['id'], $_POST['drinkid'], $_POST['name'], $_POST['img'], $_POST['primaryLiquor'], $_POST['ingredientList'], $_POST['garnish'], $_POST['glass'], $_POST['tags'], $_POST['summary'], $_POST['ingredients'], $_POST['instructions']);
  }
  if($addEdit === 'edit') {
    $sq = $scon->prepare('UPDATE drinks SET name=?,img=?,primaryLiquor=?,ingredientList=?,garnish=?,glass=?,tags=?,summary=?,ingredients=?,instructions=? WHERE userid=? AND drinkid=? LIMIT 1');
    $sq->bind_param('ssssssssssss', $_POST['name'], $_POST['img'], $_POST['primaryLiquor'], $_POST['ingredientList'], $_POST['garnish'], $_POST['glass'], $_POST['tags'], $_POST['summary'], $_POST['ingredients'], $_POST['instructions'], $_COOKIE['id'], $_POST['drinkid']);
  }
  $sq->execute();
  $sq->close();
  if(!$sq) http_response_code(500);
  else if($_POST['imgTrigger'] === '1')
    imgTrigger($_POST['drinkid']);
}


function validateDrink() {
  if($_SERVER['REQUEST_METHOD'] !== 'POST') return 1;
  $err = 11;
  $err -= isset($_POST['drinkid']) + isset($_POST['img']) + !empty($_POST['name']) + !empty($_POST['primaryLiquor']) + !empty($_POST['ingredientList']) + isset($_POST['garnish']) + isset($_POST['glass']) + isset($_POST['tags']) + isset($_POST['summary']) + isset($_POST['ingredients']) + isset($_POST['instructions']);
  if($err) return 1;
  $err += !preg_match('/^[4-9a-z]{16}$/', $_POST['drinkid']);
  $err += !is_numeric($_COOKIE['id']);
  $err += !is_numeric($_POST['img']);
  $err += str_contains($_POST['tags'], '<');

  foreach(['primaryLiquor','ingredientList','garnish','glass','tags','summary','ingredients','instructions'] as $column)
    $_POST[$column] = $_POST[$column] ?: null;
  return $err;
}


function cloneDrink() {
  global $scon, $urlPath;
  if(!sqlSafe($urlPath[3])) {http_response_code(400); echo 'Bad Username'; return;}
  if(!ctype_alnum($urlPath[4])) {http_response_code(400); echo 'Bad Drink ID'; return;}

  $res = mysqli_query($scon, 'SELECT userid FROM users WHERE username="'.$urlPath[3].'"');
  if(!$res) {http_response_code(404); echo 'Source User Not Found'; return;}
  $uid = mysqli_fetch_row($res)[0];

  $did = idGen32();
  $res = mysqli_query($scon, 'INSERT INTO drinks SELECT "'.$_COOKIE['id'].'","'.$did.'",name,img,CURRENT_TIMESTAMP(),CURRENT_TIMESTAMP(),imgRes,primaryLiquor,ingredientList,garnish,glass,tags,summary,ingredients,instructions FROM drinks WHERE userid='.$uid.' AND drinkid="'.$urlPath[4].'"');
  if(!$res) http_response_code(500);
  else echo $did;

  $usernameLC = strtolower($_COOKIE['user']);
  $userLC = strtolower($urlPath[3]);
  if(file_exists('img/'.$userLC.'/'.$urlPath[4].'.png')) {
    if(!is_dir('img/'.$usernameLC))
      mkdir('img/'.$usernameLC);
    copy('img/'.$userLC.'/'.$urlPath[4].'.png', 'img/'.$usernameLC.'/'.$did.'.png');
  }
}


function deleteDrink() {
  global $scon, $urlPath;
  if(!ctype_alnum($urlPath[3])) {http_response_code(400); echo 'Invalid Drink ID'; return;}
  $res = mysqli_query($scon, 'DELETE FROM drinks WHERE userid="'.$_COOKIE['id'].'" AND drinkid="'.$urlPath[3].'" LIMIT 1');
  if(!$res) http_response_code(500);
  else {
    if(file_exists('img/'.strtolower($_COOKIE['user']).'/'.$urlPath[3].'.png'))
      unlink('img/'.strtolower($_COOKIE['user']).'/'.$urlPath[3].'.png');
  }
}


function imageUpload() {
  global $urlPath;
  $imgData = file_get_contents('php://input');
  if($_SERVER['REQUEST_METHOD'] !== 'POST' || empty($imgData) || !preg_match('/^[4-9a-z]{16}$/', $urlPath[3])) {
    http_response_code(400);
    echo 'Missing Img Data';
    return;
  }

  $fileName = strtolower($_COOKIE['user']) . '~' . $urlPath[3];
  file_put_contents('imagesTmp/'.$fileName, file_get_contents('php://input'));

  $type = exif_imagetype('imagesTmp/'.$fileName);
  if($type !== IMAGETYPE_JPEG && $type !== IMAGETYPE_PNG && $type !== IMAGETYPE_BMP && $type !== IMAGETYPE_WEBP) {
    http_response_code(401);
    echo 'Invalid Image Type';
  }
}


function imgTrigger($drinkID) {
  global $scon, $urlPath;
  if(!$drinkID) {
    $drinkID = $urlPath[3];
    if(!preg_match('/^[4-9a-z]{16}$/', $drinkID)) {
      http_response_code(400);
      echo 'Bad Drink ID';
      return;
    }
  }

  $tmpImg = 'imagesTmp/' . strtolower($_COOKIE['user']) . '~' . $drinkID;

  if(!file_exists($tmpImg))
    return;

  list($x,$y,$type) = getimagesize($tmpImg);

  if(!in_array($type, [IMAGETYPE_JPEG, IMAGETYPE_PNG, IMAGETYPE_BMP, IMAGETYPE_WEBP], true)) {
    //unlink($tmpImg);
    if(!file_exists('img/' . strtolower($_COOKIE['user']) . '/' . $drinkID . '.png')) //img up failed after save and no pre-existing img exists
      mysqli_query($scon, 'UPDATE drinks SET img=0 WHERE userid="'.$_COOKIE['id'].'" AND drinkid="'.$drinkID.'" LIMIT 1');
    return;
  }

  $imgSize = imgResize($x,$y);
  if($imgSize)
    mysqli_query($scon, 'UPDATE drinks SET imgRes="'.$imgSize.'" WHERE userid="'.$_COOKIE['id'].'" AND drinkid="'.$drinkID.'" LIMIT 1');

  rename($tmpImg, 'images/' . strtolower($_COOKIE['user']) . '~' . $drinkID);
}


function login() { //LOGIN
  global $scon, $urlPath;
  if($_SERVER['REQUEST_METHOD'] !== 'POST') {http_response_code(400); echo 'Invalid Method'; return;}
  if(!sqlSafe($urlPath[3])) {http_response_code(400); echo 'Bad Username'; return;}

  $un = $urlPath[3];
  $pw = file_get_contents('php://input');
  $res = mysqli_query($scon, 'SELECT userid,username,passwd FROM users WHERE username="'.$un.'" LIMIT 1');

  if($res && mysqli_num_rows($res) === 1) {
    $creds = mysqli_fetch_assoc($res);
    if(password_verify('SALT'.$pw, $creds['passwd'])) {
      $cookieSettings = ['expires'=>strtotime('+120 days'), 'path'=>'/', 'secure'=>false, 'httponly'=>false, 'samesite'=>'Lax'];
      setcookie('user', $creds['username'], $cookieSettings);
      setcookie('id', $creds['userid'], $cookieSettings);
      setcookie('tok', md5('XXX'.$creds['username'].$creds['userid'].'XXX'), $cookieSettings);
    }
    else {http_response_code(401); echo 'Incorrent Password';}
  }
  else {http_response_code(401); echo 'User Not Found';}
}


function register() { //REGISTER
  global $scon, $urlPath;
  $un = $urlPath[3];
  $pw = file_get_contents('php://input');
  if($_SERVER['REQUEST_METHOD'] !== 'POST' || empty($pw)) {http_response_code(400); echo 'Invalid Method'; return;}

  if(!preg_match("/^[0-9a-zA-Z_\-]{1,31}$/", $un)) {http_response_code(400); echo 'Invalid Username'; return;}
  if(empty($pw)) {http_response_code(400); echo 'Empty Password'; return;}
  if(strlen($pw) > 127) {http_response_code(400); echo 'Password Too Long'; return;}

  $res = mysqli_query($scon, 'SELECT username FROM users WHERE username="'.$un.'" LIMIT 1');
  if(mysqli_num_rows($res) != 0) {http_response_code(401); echo 'Username Already Exists'; return;}

  //Username and email are free
  $pass = password_hash('SALT'.$pw, PASSWORD_BCRYPT, array('cost' => 10));
  mysqli_query($scon,'INSERT INTO users(username,passwd) VALUES("'.$un.'","'.$pass.'")');
  $uid = mysqli_insert_id($scon);
  $cookieSettings = ['expires'=>strtotime('+120 days'), 'path'=>'/', 'secure'=>false, 'httponly'=>false, 'samesite'=>'Lax'];
  setcookie('user', $un, $cookieSettings);
  setcookie('id', $uid, $cookieSettings);
  setcookie('tok', md5('XXX'.$un.$uid.'XXX'), $cookieSettings);
}


function export() { //Export Drinks
  global $scon;
  if(!sqlSafe($_COOKIE['id'])) {
    http_response_code(400);
    echo "Don't be lame";
    return;
  }
  $res = $scon->query('SELECT * FROM drinks WHERE userid='.$_COOKIE['id']);
  if(empty($res) || mysqli_num_rows($res) < 1) {
    http_response_code(404);
    return;
  }
  $data = mysqli_fetch_all($res, MYSQLI_ASSOC);
  $usernameLC = strtolower($_COOKIE['user']);
  foreach($data as &$d) {
    unset($d['userid']);
    $d['img'] = ($d['img'] === '1') ? 'https://cocktailkeeper.cc/img/'.$usernameLC.'/'.$d['drinkid'].'.png' : null;
  }

  header('content-disposition: attachment; filename="cocktailkeeper_drink_recipies_'.$_COOKIE['user'].'.json"');
  echo json_encode($data);
}


function cleanImages() {
  global $scon, $urlPath;
  if($_SERVER['REMOTE_ADDR'] !== '127.0.0.1') {http_response_code(403); return;}
  $startIdx; $endIdx = 0;
  $now = time();

  //tmp img up dir
  $dir = new DirectoryIterator('imagesTmp');
  foreach($dir as $img) {
    if(!$img->isFile()) continue;
    if($now - $img->getMTime() > 14400) {
      unlink($img->getPathname());
      file_put_contents('imgCleanup.log', 'imagesTmp/'.$img->getBasename()."\n", FILE_APPEND);
    }
  }

  if($urlPath[3] !== 'deep') return;

  $res = mysqli_query($scon, 'SELECT drinkid,username FROM drinks INNER JOIN users ON drinks.userid = users.userid WHERE drinks.img=1 ORDER BY username');
  $drinks = mysqli_fetch_all($res, MYSQLI_NUM);
  $drinksLen = count($drinks);
  $users = array_unique(array_column($drinks, 1));

  foreach($users as $user) {
    $startIdx = $endIdx;
    while($endIdx < $drinksLen && $drinks[$endIdx][1] === $user)
      $endIdx += 1;

    $dir = new DirectoryIterator('/tor/cocktailkeeper/'.strtolower($user));
    foreach($dir as $img) {
      if(!$img->isFile()) continue;
      $bn = $img->getBasename('.png');
      $present = 0;
      for($i = $startIdx; $i < $endIdx; $i++) {
        $present += ($bn === $drinks[$i][0]);
      }
      if(!$present && $now - $img->getMTime() > 14400) {
        unlink($img->getPathname());
        file_put_contents('imgCleanup.log', $user.'/'.$img->getBasename()."\n", FILE_APPEND);
      }
    }
  }
}


function idGen32() {
  $chars = '';
  $bytes = random_bytes(16);

  for($i = 0; $i < 16; $i++) {
    $byte = ord($bytes[$i]) & 31;
    $byte += ($byte > 9) ? 87 : 48;
    $chars .= chr($byte);
  }
  return $chars;
}


function sqlSafe($str) {
  foreach(['\\', '"', ';', '$', '<'] as $a) {
    if(stripos($str, $a) !== false) return false;
  }
  return true;
}


function imgResize($dimX, $dimY) {
  if(!$dimX || !$dimY || !is_numeric($dimX) || !is_numeric($dimY))
    return null;
  $scalePc = $dimY / 156;
  $scalePcInt = (int) $scalePc;
  $scalePcInt += ($scalePc - $scalePcInt > 0.75);
  if($scalePc > 1.3 && $scalePc <= 1.75)
    $scalePcInt = 96; // 2/3

  while($scalePcInt > 1) {

    switch($scalePcInt) {
      case 2: $dimY /= 2; $dimX /= 2; break; // 1/2
      case 3: $dimY /= 3; $dimX /= 3; break; // 1/3
      case 4:
      case 5: $dimY /= 4; $dimX /= 4; break; // 1/4
      case 6:
      case 7: $dimY /= 6; $dimX /= 6; break; // 1/6
      case 8: $dimY /= 8; $dimX /= 8; break; // 1/8
      case 9:
      case 10: $dimY /= 9; $dimX /= 9; break; // 1/9
      case 11:
      case 12:
      case 13:
      case 14: $dimY /= 12; $dimX /= 12; break; // 1/12
      case 96: $dimY = ($dimY * 2) / 3; $dimX = ($dimX * 2) / 3; break; // 2/3
      default: $dimY /= 16; $dimX /= 16; break; // 1/16
    }

    $dimY = (int) $dimY;
    $dimX = (int) $dimX;
    $scalePc = $dimY / 156;
    $scalePcInt = (int) $scalePc;
    $scalePcInt += ($scalePc - $scalePcInt > 0.75);
    if($scalePc > 1.3 && $scalePc <= 1.75)
      $scalePcInt = 96; // 2/3

  }

  $dimX = ($dimX > $dimY) ? $dimY : $dimX;

  return $dimX .'x'. $dimY;
}

?>
