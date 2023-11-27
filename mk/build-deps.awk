{
  if (match($0, "^"target))
  {
    for (i=2; i<=NF; i++)
    {
      if ($i ~ /.c$/)
      {
        sub(".c$", ".o", $i);
        sub("src/", "$(BD_OBJ)/", $i);
        print $i
      }
    }
  }
}
