<?php
/**
 * This implements the Login page class
 *
 * PHP Version 7
 *
 * @category API
 * @package  Loris
 * @author   Dave MacFarlane <dave.macfarlane@mcin.ca>
 * @license  Loris license
 * @link     https://github.com/aces/Loris
 */
namespace LORIS\api;

use \Psr\Http\Message\ServerRequestInterface;
use \Psr\Http\Message\ResponseInterface;


/**
 * A class for handling the api/v????/login endpoint.
 *
 * @category API
 * @package  Loris
 * @author   Dave MacFarlane <dave.macfarlane@mcin.ca>
 * @license  Loris license
 * @link     https://github.com/aces/Loris
 */
class Login extends APIEndpoint
{
    /**
     * All users have access to the login endpoint to try and login.
     *
     * @return boolean true if access is permitted
     */
    function _hasAccess()
    {
        // Anyone can try and login. Even you.
        return true;
    }

    /**
     * Return which methods are supported by this endpoint.
     *
     * Login can only be POSTed to
     *
     * @return array supported HTTP methods
     */
    protected function allowedMethods() : array
    {
        return ['POST'];
    }

    /**
     * Versions of the LORIS API which are supported by this
     * endpoint.
     *
     * Login has existed since v0.0.1 of the API and has not
     * changed
     *
     * @return array a list of supported API versions.
     */
    protected function supportedVersions() : array
    {
        return [
                "v0.0.1",
                "v0.0.2",
                "v0.0.3-dev",
               ];
    }

    /**
     * Handles a login request
     *
     * @param ServerRequestInterface $request The incoming PSR7 request
     *
     * @return ResponseInterface The outgoing PSR7 response
     */
    public function handle(ServerRequestInterface $request) : ResponseInterface
    {
        $requestdata = json_decode($request->getBody(), true);
        if (!isset($requestdata['username']) || !isset($requestdata['password'])) {
            return (new \LORIS\Http\Response())
                ->withBody(
                    new \LORIS\Http\StringStream(
                        '{ "error" : Missing username or password" }'
                    )
                )
                ->withStatus(400);
        }

        $user     = $requestdata['username'];
        $password = $requestdata['password'];

        $login = $this->getLoginAuthenticator();

        if ($login->passwordAuthenticate($user, $password, false)) {
            $token = $this->getEncodedToken($user);
            if (!empty($token)) {
                return (new \LORIS\Http\Response())
                    ->withBody(
                        new \LORIS\Http\StringStream(
                            json_encode(['token' => $token])
                        )
                    )
                    ->withHeader("Content-Type", "application/json")
                    ->withStatus(200);
            } else {
                return (new \LORIS\Http\Response())
                    ->withBody(
                        new \LORIS\Http\StringStream(
                            ['error' => 'Unacceptable JWT key']
                        )
                    )
                    ->withHeader("Content-Type", "application/json")
                    ->withStatus(500);
            }
        }
        return (new \LORIS\Http\Response())
            ->withBody(
                new \LORIS\Http\StringStream(
                    json_encode(array("error" => $login->_lastError))
                )
            )
            ->withHeader("Content-Type", "application/json")
            ->withStatus(401);
    }

    /**
     * Get the SinglePointLogin class used to authenticate this request
     * in a separate function so that it can be mocked out for testing.
     *
     * @return \SinglePointLogin Loris Authenticator
     */
    function getLoginAuthenticator()
    {
        return new \SinglePointLogin();
    }

    /**
     * Return a valid JWT encoded identification token for the user
     *
     * @param string $user The user to return an identification token for
     *
     * @return string JWT encoded token
     */
    function getEncodedToken($user)
    {
        $factory = \NDB_Factory::singleton();
        $config  = $factory->config();

        $www     = $config->getSetting("www");
        $baseURL = $www['url'];

        $token = array(
            // JWT related tokens to for the JWT library to validate
                  "iss"  => $baseURL,
                  "aud"  => $baseURL,
            // Issued at
                  "iat"  => time(),
                  "nbf"  => time(),
            // Expire in 1 day
                  "exp"  => time() + 86400,
            // Additional payload data
                  "user" => $user,
                 );

        $key = $config->getSetting("JWTKey");
        if (!self::isKeyStrong($key)) {
            return "";
        }
        return \Firebase\JWT\JWT::encode($token, $key, "HS256");
    }

    /**
     * Verify key meets cryptographic strength requirements
     *
     * @param string $key The JWT key to verify
     *
     * @return boolean Key passes strength test
     */
    static function isKeyStrong($key)
    {
        // Note: this code adapted from User::isPasswordStrong
        $CharTypes = 0;
        // less than 20 characters
        if (strlen($key) < 20) {
            return false;
        }
        // nothing but letters
        if (!preg_match('/[^A-Za-z]/', $key)) {
            return false;
        }
        // nothing but numbers
        if (!preg_match('/[^0-9]/', $key)) {
            return false;
        }
        // preg_match returns 1 on match, 0 on non-match
        $CharTypes += preg_match('/[0-9]+/', $key);
        $CharTypes += preg_match('/[A-Za-z]+/', $key);
        $CharTypes += preg_match('/[!\\\$\^@#%&\*\(\)]+/', $key);
        if ($CharTypes < 3) {
            return false;
        }

        return true;
    }
}
