import React, { useState } from "react";

const Login: React.FC = () => {
  const [username, setUsername] = useState("");
  const [password, setPassword] = useState("");
  const [error, setError] = useState("");

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    if (!username || !password) {
      setError("Both fields are required.");
      return;
    }
    setError("");
    console.log("Logging in with:", { username, password });
  };

  return (
    <div className="flex items-center justify-center min-h-screen bg-gray-100">
      <form
        onSubmit={handleSubmit}
        className="bg-white p-6 rounded-2xl shadow-lg w-80"
      >
        <h2 className="text-xl font-bold mb-4 text-center">Login</h2>
        {error && <p className="text-red-500 text-sm mb-2">{error}</p>}
        <label className="block mb-2">
          Email
          <input
            type="email"
            className="w-full border rounded-lg p-2 mt-1"
            value={username}
            onChange={(e) => setUsername(e.target.value)}
          />
        </label>
        <label className="block mb-4">
          Password
          <input
            type="password"
            className="w-full border rounded-lg p-2 mt-1"
            value={password}
            onChange={(e) => setPassword(e.target.value)}
          />
        </label>
        <button
          type="submit"
          className="w-full bg-blue-500 text-white py-2 rounded-lg hover:bg-blue-600 transition"
        >
          Log In
        </button>
      </form>
    </div>
  );
};

export default Login;